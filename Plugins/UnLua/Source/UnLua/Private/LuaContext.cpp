// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "LuaContext.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "UnLuaEx.h"
#include "UnLuaManager.h"
#include "UnLuaInterface.h"
#include "UnLuaDelegates.h"
#include "UnLuaDebugBase.h"
#include "UEObjectReferencer.h"
#include "CollisionHelper.h"
#include "DelegateHelper.h"
#include "ReflectionUtils/PropertyCreator.h"
#include "DefaultParamCollection.h"
#include "UnLuaFunctionLibrary.h"
#include "ReflectionUtils/ReflectionRegistry.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

static constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags::AsyncLoading | EInternalObjectFlags::Async;

/**
 * Statically exported callback for 'Hotfix'
 */
bool OnModuleHotfixed(const char* ModuleName)
{
    if (!GLuaCxt->IsEnable() || !ModuleName)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    bool bSuccess = GLuaCxt->GetUnLuaManager()->OnModuleHotfixed(UTF8_TO_TCHAR(ModuleName));
#if !UE_BUILD_SHIPPING
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Failed to update module!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
#endif
    return bSuccess;
}

EXPORT_FUNCTION(bool, OnModuleHotfixed, const char*)


FLuaContext* GLuaCxt = nullptr;

/**
 * Create GLuaCxt
 */
FLuaContext* FLuaContext::Create()
{
    if (!GLuaCxt)
    {
        static FLuaContext Context;
        GLuaCxt = &Context;
    }
    return GLuaCxt;
}

/**
 * Register different engine delegates
 */
void FLuaContext::RegisterDelegates()
{
    FWorldDelegates::OnWorldCleanup.AddRaw(this, &FLuaContext::OnWorldCleanup);
    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FLuaContext::OnPostEngineInit);   // called before FCoreDelegates::OnFEngineLoopInitComplete.Broadcast(), after GEngine->Init(...)
    FCoreDelegates::OnPreExit.AddRaw(this, &FLuaContext::OnPreExit);                 // called before StaticExit()

    FCoreDelegates::OnHandleSystemError.AddRaw(this, &FLuaContext::OnCrash);
    FCoreDelegates::OnHandleSystemEnsure.AddRaw(this, &FLuaContext::OnCrash);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FLuaContext::PostLoadMapWithWorld);
    //FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddRaw(this, &FLuaContext::OnPreGarbageCollect);

#if WITH_EDITOR
    FEditorDelegates::PreBeginPIE.AddRaw(this, &FLuaContext::PreBeginPIE);
    FEditorDelegates::PostPIEStarted.AddRaw(this, &FLuaContext::PostPIEStarted);
    FEditorDelegates::PrePIEEnded.AddRaw(this, &FLuaContext::PrePIEEnded);
#endif

    GUObjectArray.AddUObjectCreateListener(this);    // add listener for creating UObject
    GUObjectArray.AddUObjectDeleteListener(this);    // add listener for deleting UObject
}

/**
 * Create Lua state (main thread) and register/create base libs/tables/classes
 */
void FLuaContext::CreateState()
{
    if (Env)
        return;

    Env = MakeShared<UnLua::FLuaEnv>();
    Env->Initialize();
}

/**
 * Enable UnLua
 */
void FLuaContext::SetEnable(bool InEnable)
{
    if (InEnable)
    {
        Initialize();
    }
    else
    {
        Cleanup(true);
    }
}

/**
 * Is UnLua enabled?
 */
bool FLuaContext::IsEnable() const
{
    return Env.IsValid();
}

/**
 * Try to bind Lua module for a UObject
 */
bool FLuaContext::TryToBindLua(UObject* Object)
{
    if (!bEnable || !UnLua::IsUObjectValid(Object))
        return false;

    return Env->TryBind(Object);
}

/**
 * Callback for FWorldDelegates::OnWorldTickStart
 */
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 23)
void FLuaContext::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
#else
void FLuaContext::OnWorldTickStart(ELevelTick TickType, float DeltaTime)
#endif
{
    if (!Env)
        return;

    const auto Manager = Env->GetManager();
    if (!Manager)
        return;

    for (UInputComponent* InputComponent : CandidateInputComponents)
    {
        if (!InputComponent->IsRegistered() || InputComponent->IsPendingKill())
        {
            continue;
        }

        AActor* Actor = Cast<AActor>(InputComponent->GetOuter());
        Manager->ReplaceInputs(Actor, InputComponent);                              // try to replace/override input events
    }

    CandidateInputComponents.Empty();
    FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
}

/**
 * Callback for FWorldDelegates::OnWorldCleanup
 */
void FLuaContext::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    if (!World || !bEnable)
    {
        return;
    }

    World->RemoveOnActorSpawnedHandler(OnActorSpawnedHandle);

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 23)
    Cleanup(IsEngineExitRequested(), World);                    // clean up
#else
    Cleanup(GIsRequestingExit, World);                          // clean up
#endif
}

/**
 * Callback for FCoreDelegates::OnPostEngineInit
 */
void FLuaContext::OnPostEngineInit()
{
    CreateDefaultParamCollection();                 // create data for default parameters of UFunctions

#if WITH_EDITOR
    UGameViewportClient* GameViewportClient = GEngine->GameViewport;
    if (GameViewportClient)
    {
        GameViewportClient->OnGameViewportInputKey().BindRaw(this, &FLuaContext::OnGameViewportInputKey);   // bind a default input event
    }
#endif
}

/**
 * Callback for FCoreDelegates::OnPreExit
 */
void FLuaContext::OnPreExit()
{
    Cleanup(true);                                  // full clean up
}


/**
 * Callback for FCoreDelegates::OnHandleSystemError and FCoreDelegates::OnHandleSystemEnsure
 */
void FLuaContext::OnCrash()
{
    if (!IsInGameThread())
        return;

    if (!Env)
        return;

    const auto L = Env->GetMainState();
    const FString Msg = UnLua::GetLuaCallStack(L);
    UE_LOG(LogUnLua, Error, TEXT("%s"), *Msg);
    GLog->Flush();
}


/**
 * Callback for FCoreUObjectDelegates::PostLoadMapWithWorld
 */
void FLuaContext::PostLoadMapWithWorld(UWorld* World)
{
    if (!World || !bEnable)
    {
        return;
    }

    const auto Manager = Env->GetManager();
    Manager->OnMapLoaded(World);

    // !!!Fix!!!
    // when world is cleanup, this need to remove
    // register callback for spawning an actor
    OnActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(Manager, &UUnLuaManager::OnActorSpawned));
}

#if WITH_EDITOR
/**
 * Callback for FEditorDelegates::PreBeginPIE
 */
void FLuaContext::PreBeginPIE(bool bIsSimulating)
{
#if AUTO_UNLUA_STARTUP
    SetEnable(true);
#endif

    UGameViewportClient* GameViewportClient = GEngine->GameViewport;
    if (GameViewportClient)
    {
        GameViewportClient->OnGameViewportInputKey().BindRaw(this, &FLuaContext::OnGameViewportInputKey);   // bind a default input event
    }
}

/**
 * Callback for FEditorDelegates::PostPIEStarted
 */
void FLuaContext::PostPIEStarted(bool bIsSimulating)
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine)
    {
        PostLoadMapWithWorld(EditorEngine->PlayWorld);
    }
}

/**
 * Callback for FEditorDelegates::PrePIEEnded
 */
void FLuaContext::PrePIEEnded(bool bIsSimulating)
{
    // close lua env alwaylls
    SetEnable(false);
}

#endif

/**
 * Callback when a UObjectBase (not full UObject) is created
 */
void FLuaContext::NotifyUObjectCreated(const UObjectBase* InObject, int32 Index)
{
    {
        FScopeLock Lock(&Async2MainCS);
#if UNLUA_ENABLE_DEBUG != 0
        UObjPtr2Name.Add(const_cast<UObjectBase*>(InObject), InObject->GetFName().ToString());
#endif
    }

    if (!bEnable)
    {
        return;
    }

#if WITH_EDITOR
    // Don't bind during cook
    if (GIsCookerLoadingPackage)
    {
        return;
    }
#endif

    // try to bind a Lua module for the object
    UObject* Object = (UObject*)InObject;
    TryToBindLua(Object);

    // special handling for UInputComponent
    if (!Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && Object->IsA<UInputComponent>())
    {
        AActor* Actor = Cast<APlayerController>(Object->GetOuter());
        if (!Actor)
        {
            Actor = Cast<APawn>(Object->GetOuter());
        }
        if (Actor && Actor->GetLocalRole() >= ROLE_AutonomousProxy)
        {
            //!!!Fix!!!
            // when tick start processing, inputcomponent may be invald or changeing
            CandidateInputComponents.AddUnique((UInputComponent*)InObject);
            if (OnWorldTickStartHandle.IsValid())
            {
                FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
            }
            OnWorldTickStartHandle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FLuaContext::OnWorldTickStart);
        }
    }
}

/**
 * Callback when a UObjectBase (not full UObject) is deleted
 */
void FLuaContext::NotifyUObjectDeleted(const UObjectBase* InObject, int32 Index)
{
    if (!bEnable)
    {
        FScopeLock Lock(&Async2MainCS);

#if UNLUA_ENABLE_DEBUG != 0
        UObjPtr2Name.Remove(InObject);
#endif

        return;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("NotifyUObjectDeleted : %s,%p"), *UObjPtr2Name[InObject], InObject);
#endif

    bool bClass = GReflectionRegistry.NotifyUObjectDeleted(InObject);
    GetUnLuaManager()->NotifyUObjectDeleted(InObject, bClass);
    FDelegateHelper::NotifyUObjectDeleted((UObject*)InObject);

    if (CandidateInputComponents.Num() > 0)
    {
        int32 NumRemoved = CandidateInputComponents.Remove((UInputComponent*)InObject);
        if (NumRemoved > 0 && CandidateInputComponents.Num() < 1)
        {
            FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
        }
    }

    FScopeLock Lock(&Async2MainCS);
    UObjPtr2Name.Remove(InObject);
}


/**
 * Callback when a GUObjectArray is deleted
 */
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
void FLuaContext::OnUObjectArrayShutdown()
{
}
#endif

UUnLuaManager* FLuaContext::GetUnLuaManager()
{
    return Env ? Env->GetManager() : nullptr;
}


FLuaContext::FLuaContext()
    : Env(nullptr), bEnable(false)
{
}

FLuaContext::~FLuaContext()
{
    Cleanup(true);

    if (Env)
        Env = nullptr;

    // when exiting, remove listeners for creating/deleting UObject
    GUObjectArray.RemoveUObjectCreateListener(GLuaCxt);
    GUObjectArray.RemoveUObjectDeleteListener(GLuaCxt);

    FScopeLock Lock(&Async2MainCS);

#if UNLUA_ENABLE_DEBUG != 0
    UObjPtr2Name.Empty();
#endif
}

/**
 * Initialize UnLua
 */
void FLuaContext::Initialize()
{
    if (!bEnable)
    {
        CreateState();  // create Lua main thread

        if (Env)
        {
            GPropertyCreator.Cleanup();
            bEnable = true;
            FUnLuaDelegates::OnLuaContextInitialized.Broadcast();
        }
    }
}

/**
 * Clean up UnLua
 */
void FLuaContext::Cleanup(bool bFullCleanup, UWorld* World)
{
    if (!bEnable)
    {
        return;
    }

    if (Env)
    {
        FUnLuaDelegates::OnPreLuaContextCleanup.Broadcast(bFullCleanup);

        if (!bFullCleanup)
        {
            Env->GC();
        }
        else
        {
            bEnable = false;
            Env = nullptr;

            // clean ue side modules,es static data structs
            FCollisionHelper::Cleanup();                        // clean up collision helper stuff

            GObjectReferencer.Cleanup();                        // clean up object referencer

            FDelegateHelper::Cleanup(bFullCleanup);                 // clean up delegates

            GPropertyCreator.Cleanup();                             // clean up dynamically created UProperties

            GReflectionRegistry.Cleanup();                      // clean up reflection registry

            CandidateInputComponents.Empty();
            FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
        }

        FUnLuaDelegates::OnPostLuaContextCleanup.Broadcast(bFullCleanup);
    }
}

/**
 * Build-in input event for 'Hotfix'
 */
bool FLuaContext::OnGameViewportInputKey(FKey InKey, FModifierKeysState ModifierKeyState, EInputEvent EventType)
{
    if (!bEnable)
    {
        return false;
    }
    if (InKey == EKeys::L && ModifierKeyState.IsControlDown() && EventType == IE_Released)
    {
        UUnLuaFunctionLibrary::HotReload();
        return true;
    }
    return false;
}
