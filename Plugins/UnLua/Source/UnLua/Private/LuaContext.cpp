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
#include "UEReflectionUtils.h"
#include "CollisionHelper.h"
#include "DelegateHelper.h"
#include "PropertyCreator.h"
#include "DefaultParamCollection.h"
#include "Interfaces/IPluginManager.h"

#if WITH_EDITOR
#include "Editor.h"
#include "GameDelegates.h"
#endif

#if UE_BUILD_TEST
#include "Tests/UnLuaPerformanceTestProxy.h"

void RunPerformanceTest(UWorld *World)
{
    if (!World)
    {
        return;
    }
    static AActor *PerformanceTestProxy = World->SpawnActor(AUnLuaPerformanceTestProxy::StaticClass());
}
#endif

static UUnLuaManager *SManager = nullptr;

/**
 * Statically exported callback for 'Hotfix'
 */
bool OnModuleHotfixed(const char *ModuleName)
{
    if (!SManager || !ModuleName)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    bool bSuccess = SManager->OnModuleHotfixed(ANSI_TO_TCHAR(ModuleName));
#if !UE_BUILD_SHIPPING
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Failed to update module!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
#endif
    return bSuccess;
}

EXPORT_FUNCTION(bool, OnModuleHotfixed, const char*)


FLuaContext *GLuaCxt = nullptr;

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
    if (IsRunningCommandlet())
    {
        return;
    }

    if (bDelegatesRegistered)
    {
        return;
    }

    bDelegatesRegistered = true;

    FWorldDelegates::OnWorldCleanup.AddRaw(GLuaCxt, &FLuaContext::OnWorldCleanup);
    FWorldDelegates::OnPostWorldCleanup.AddRaw(GLuaCxt, &FLuaContext::OnPostWorldCleanup);
    FWorldDelegates::OnPreWorldInitialization.AddRaw(GLuaCxt, &FLuaContext::OnPreWorldInitialization);
    FWorldDelegates::OnPostWorldInitialization.AddRaw(GLuaCxt, &FLuaContext::OnPostWorldInitialization);
    FCoreDelegates::OnPostEngineInit.AddRaw(GLuaCxt, &FLuaContext::OnPostEngineInit);   // called before FCoreDelegates::OnFEngineLoopInitComplete.Broadcast(), after GEngine->Init(...)
    FCoreDelegates::OnPreExit.AddRaw(GLuaCxt, &FLuaContext::OnPreExit);                 // called before StaticExit()
    FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(GLuaCxt, &FLuaContext::OnAsyncLoadingFlushUpdate);
    FCoreDelegates::OnHandleSystemError.AddRaw(GLuaCxt, &FLuaContext::OnCrash);
    FCoreDelegates::OnHandleSystemEnsure.AddRaw(GLuaCxt, &FLuaContext::OnCrash);
    FCoreUObjectDelegates::PreLoadMap.AddRaw(GLuaCxt, &FLuaContext::PreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(GLuaCxt, &FLuaContext::PostLoadMapWithWorld);

#if WITH_EDITOR
    // delegates for PIE
    FEditorDelegates::PreBeginPIE.AddRaw(GLuaCxt, &FLuaContext::PreBeginPIE);
    FEditorDelegates::BeginPIE.AddRaw(GLuaCxt, &FLuaContext::BeginPIE);
    FEditorDelegates::PostPIEStarted.AddRaw(GLuaCxt, &FLuaContext::PostPIEStarted);
    FEditorDelegates::PrePIEEnded.AddRaw(GLuaCxt, &FLuaContext::PrePIEEnded);
    FEditorDelegates::EndPIE.AddRaw(GLuaCxt, &FLuaContext::EndPIE);
    FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(GLuaCxt, &FLuaContext::OnEndPlayMap);
#endif
}

/**
 * Create Lua state (main thread) and register/create base libs/tables/classes
 */
void FLuaContext::CreateState()
{
    if (IsRunningCommandlet())
    {
        return;
    }

    if (!L)
    {
#if WITH_EDITOR
        // load Lua dynamic lib under 'WITH_EDITOR' mode
        if (!LuaHandle)
        {
#if PLATFORM_WINDOWS
            FString PlatformName(TEXT("Win64"));
            FString LibName(TEXT("Lua.dll"));
#elif PLATFORM_MAC
            FString PlatformName(TEXT("Mac"));
            FString LibName(TEXT("liblua.dylib"));
#endif
            FString LibPath = FString::Printf(TEXT("%s/Source/ThirdParty/Lua/binaries/%s/%s"), *IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetBaseDir(), *PlatformName, *LibName);
            if (FPaths::FileExists(LibPath))
            {
                LuaHandle = FPlatformProcess::GetDllHandle(*LibPath);
                if (!LuaHandle)
                {
                    UE_LOG(LogUnLua, Log, TEXT("%s: failed to load %s!"), ANSI_TO_TCHAR(__FUNCTION__), *LibPath);
                    return;
                }
            }
        }
#endif

        L = lua_newstate(FLuaContext::LuaAllocator, nullptr);       // create main Lua thread
        check(L);
        luaL_openlibs(L);                                           // open all standard Lua libraries

        lua_pushstring(L, "ObjectMap");                             // create weak table 'ObjectMap'
        CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, "StructMap");                             // create weak table 'StructMap'
        CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, "ScriptContainerMap");                    // create weak table 'ScriptContainerMap'
        CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, "ArrayMap");                              // create weak table 'ArrayMap'
        CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        CreateNamespaceForUE(L);                                    // create 'UE4' namespace (table)

        // register global Lua functions
        lua_register(L, "RegisterEnum", Global_RegisterEnum);
        lua_register(L, "RegisterClass", Global_RegisterClass);
        lua_register(L, "GetUProperty", Global_GetUProperty);
        lua_register(L, "SetUProperty", Global_SetUProperty);
        lua_register(L, "LoadObject", Global_LoadObject);
        lua_register(L, "LoadClass", Global_LoadClass);
        lua_register(L, "NewObject", Global_NewObject);

        lua_register(L, "UEPrint", Global_Print);
        if (FPlatformProperties::RequiresCookedData())
        {
            lua_register(L, "require", Global_Require);             // override 'require' when running with cooked data
        }

        // register collision related enums
        RegisterECollisionChannel(L);
        RegisterEObjectTypeQuery(L);
        RegisterETraceTypeQuery(L);

#if UE_BUILD_TEST
        lua_gc(L, LUA_GCSTOP, 0);
#else
        if (FUnLuaDelegates::ConfigureLuaGC.IsBound())
        {
            FUnLuaDelegates::ConfigureLuaGC.Execute(L);
        }
        else
        {
            // default Lua GC config in UnLua
            lua_gc(L, LUA_GCSETPAUSE, 100);
            lua_gc(L, LUA_GCSETSTEPMUL, 5000);
        }
#endif

        // add new package path
        FString LuaSrcPath = GLuaSrcFullPath + TEXT("?.lua");
        AddPackagePath(L, TCHAR_TO_UTF8(*LuaSrcPath));

        FUnLuaDelegates::OnPreStaticallyExport.Broadcast();

        RegisterClass(L, "UClass", "UObject");                      // register base class

        // register statically exported classes
        for (TMap<FName, UnLua::IExportedClass*>::TIterator It(ExportedNonReflectedClasses); It; ++It)
        {
            It.Value()->Register(L);
        }

        // register statically exported global functions
        for (UnLua::IExportedFunction *Function : ExportedFunctions)
        {
            Function->Register(L);
        }

        // register statically exported enums
        for (UnLua::IExportedEnum *Enum : ExportedEnums)
        {
            Enum->Register(L);
        }

        FUnLuaDelegates::OnLuaStateCreated.Broadcast(L);
    }
}

/**
 * Enable UnLua
 */
void FLuaContext::SetEnable(bool InEnable)
{
    if (InEnable)
    {
        CreateState();
    }
    else
    {
        Cleanup(true);
    }
    bEnable = InEnable;
    if (bEnable)
    {
        Initialize();
    }
}

/**
 * Is UnLua enabled?
 */
bool FLuaContext::IsEnable() const
{
    return L && bEnable && bInitialized;
}

/**
 * Statically export a global functions
 */
bool FLuaContext::ExportFunction(UnLua::IExportedFunction *Function)
{
    if (Function)
    {
        ExportedFunctions.Add(Function);
        return true;
    }
    return false;
}

/**
 * Statically export an enum
 */
bool FLuaContext::ExportEnum(UnLua::IExportedEnum *Enum)
{
    if (Enum)
    {
        ExportedEnums.Add(Enum);
        return true;
    }
    return false;
}

/**
 * Statically export a class
 */
bool FLuaContext::ExportClass(UnLua::IExportedClass *Class)
{
    if (Class)
    {
        TMap<FName, UnLua::IExportedClass*> &ExportedClasses = Class->IsReflected() ? ExportedReflectedClasses : ExportedNonReflectedClasses;
        ExportedClasses.Add(Class->GetName(), Class);
        return true;
    }
    return false;
}

/**
 * Find a statically exported class
 */
UnLua::IExportedClass* FLuaContext::FindExportedClass(FName Name)
{
    UnLua::IExportedClass **Class = ExportedReflectedClasses.Find(Name);
    if (Class)
    {
        return *Class;
    }
    Class = ExportedNonReflectedClasses.Find(Name);
    return Class ? *Class : nullptr;
}

/**
 * Find a statically exported reflected class
 */
UnLua::IExportedClass* FLuaContext::FindExportedReflectedClass(FName Name)
{
    UnLua::IExportedClass **Class = ExportedReflectedClasses.Find(Name);
    return Class ? *Class : nullptr;
}

/**
 * Add a type interface
 */
bool FLuaContext::AddTypeInterface(FName Name, TSharedPtr<UnLua::ITypeInterface> TypeInterface)
{
    if (Name == NAME_None || !TypeInterface)
    {
        return false;
    }

    TSharedPtr<UnLua::ITypeInterface> *TypeInterfacePtr = TypeInterfaces.Find(Name);
    if (!TypeInterfacePtr)
    {
        TypeInterfaces.Add(Name, TypeInterface);
    }
    return true;
}

/**
 * Find a type interface
 */
TSharedPtr<UnLua::ITypeInterface> FLuaContext::FindTypeInterface(FName Name)
{
    TSharedPtr<UnLua::ITypeInterface> *TypeInterfacePtr = TypeInterfaces.Find(Name);
    return TypeInterfacePtr ? *TypeInterfacePtr : TSharedPtr<UnLua::ITypeInterface>();
}

/**
 * Try to bind Lua module for a UObject
 */
bool FLuaContext::TryToBindLua(UObjectBaseUtility *Object)
{
    if (!bEnable || !Object || !Manager)
    {
        return false;
    }

#if WITH_EDITOR
    if (GIsEditor && !bIsPIE)
    {
        return false;
    }
#endif

    static UClass *InterfaceClass = UUnLuaInterface::StaticClass();
    if (!Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))           // filter out CDO and ArchetypeObjects
    {
        check(!Object->IsPendingKill());
        UClass *Class = Object->GetClass();
        if (Class->IsChildOf<UPackage>() || Class->IsChildOf<UClass>())             // filter out UPackage and UClass
        {
            return false;
        }
        if (Class->ImplementsInterface(InterfaceClass))                             // static binding
        {
            UFunction *Func = Class->FindFunctionByName(FName("GetModuleName"));    // find UFunction 'GetModuleName'. hard coded!!!
            if (Func)
            {
                if (Func->GetNativeFunc() && IsInGameThread())
                {
                    FString ModuleName;
                    UObject *DefaultObject = Class->GetDefaultObject();             // get CDO
                    DefaultObject->UObject::ProcessEvent(Func, &ModuleName);        // force to invoke UObject::ProcessEvent(...)
                    UClass *OuterClass = Func->GetOuterUClass();                    // get UFunction's outer class
                    Class = OuterClass == InterfaceClass ? Class : OuterClass;      // select the target UClass to bind Lua module
                    if (ModuleName.Len() < 1)
                    {
                        ModuleName = Class->GetName();
                    }
                    return Manager->Bind(Object, Class, *ModuleName, GLuaDynamicBinding.InitializerTableRef);   // bind!!!
                }
                else
                {
                    if (IsAsyncLoading())
                    {
                        // check FAsyncLoadingThread::IsMultithreaded()?
                        FScopeLock Lock(&CandidatesCS);
                        Candidates.Add((UObject*)Object);                           // mark the UObject as a candidate
                    }
                }
            }
        }
        else if (GLuaDynamicBinding.IsValid(Class))                                 // dynamic binding
        {
            return Manager->Bind(Object, Class, *GLuaDynamicBinding.ModuleName, GLuaDynamicBinding.InitializerTableRef);
        }
    }
    return false;
}

/**
 * Callback for FWorldDelegates::OnWorldTickStart
 */
#if ENGINE_MINOR_VERSION > 23	
void FLuaContext::OnWorldTickStart(UWorld *World, ELevelTick TickType, float DeltaTime)
#else
void FLuaContext::OnWorldTickStart(ELevelTick TickType, float DeltaTime)
#endif
{
    if (!Manager)
    {
        return;
    }

    for (UInputComponent *InputComponent : CandidateInputComponents)
    {
        if (!InputComponent->IsRegistered() || InputComponent->IsPendingKill())
        {
            continue;
        }

        AActor *Actor = Cast<AActor>(InputComponent->GetOuter());
        Manager->ReplaceInputs(Actor, InputComponent);                              // try to replace/override input events
    }

    CandidateInputComponents.Empty();
    FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
}

/**
 * Callback for FWorldDelegates::OnWorldCleanup
 */
void FLuaContext::OnWorldCleanup(UWorld *World, bool bSessionEnded, bool bCleanupResources)
{
    if (!World || (GIsEditor ? !World->IsGameWorld() : (!GWorld || GWorld != World)) || !bEnable)
    {
        return;
    }

    World->RemoveOnActorSpawnedHandler(OnActorSpawnedHandle);

    if (World->PersistentLevel && World->PersistentLevel->OwningWorld == World)
    {
        bIsInSeamlessTravel = World->IsInSeamlessTravel();
    }
#if ENGINE_MINOR_VERSION > 23	
    Cleanup(IsEngineExitRequested(), World);                    // clean up	
#else	
    Cleanup(GIsRequestingExit, World);                          // clean up
#endif

#if WITH_EDITOR
    int32 Index = LoadedWorlds.Find(World);
    if (Index != INDEX_NONE)
    {
        LoadedWorlds.RemoveAt(Index);
    }
#endif
}

/**
 * Callback for FWorldDelegates::OnPostWorldCleanup
 */
void FLuaContext::OnPostWorldCleanup(UWorld *World, bool bSessionEnded, bool bCleanupResources)
{
    if (!World || (GIsEditor ? !World->IsGameWorld() : (!GWorld || GWorld != World)) || !bEnable)
    {
        return;
    }

    if (NextMap.Len() > 0)
    {
        Initialize();
    }
}

/**
 * Callback for FWorldDelegates::OnPreWorldInitialization
 */
void FLuaContext::OnPreWorldInitialization(UWorld *World, const UWorld::InitializationValues)
{
#if WITH_EDITOR
    if (!World || !World->IsGameWorld() || !bEnable)
    {
        return;
    }

    ENetMode NetMode = World->GetNetMode();
    if (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer)
    {
        ServerWorld = World;
    }
#endif
}

/**
 * Callback for FWorldDelegates::OnPostWorldInitialization
 */
void FLuaContext::OnPostWorldInitialization(UWorld *World, const UWorld::InitializationValues)
{
    if (!World || GIsEditor ? !World->IsGameWorld() : (!GWorld || GWorld != World))
    {
        return;
    }

    NextMap.Empty();

    if (!bEnable)
    {
        return;
    }
}

/**
 * Callback for FCoreDelegates::OnPostEngineInit
 */
void FLuaContext::OnPostEngineInit()
{
#if AUTO_UNLUA_STARTUP
    if (!GIsEditor)
    {
        SetEnable(true);
    }
#endif

    // create UnLuaManager and add it to root
    Manager = NewObject<UUnLuaManager>();
    Manager->AddToRoot();
    SManager = Manager;

    CreateDefaultParamCollection();                 // create data for default parameters of UFunctions

#if WITH_EDITOR
    if (!GIsEditor)
    {
        UGameViewportClient *GameViewportClient = GEngine->GameViewport;
        if (GameViewportClient)
        {
            GameViewportClient->OnGameViewportInputKey().BindRaw(this, &FLuaContext::OnGameViewportInputKey);   // bind a default input event
        }
    }
#endif
}

/**
 * Callback for FCoreDelegates::OnPreExit
 */
void FLuaContext::OnPreExit()
{
    Cleanup(true);                                  // full clean up

    if (Manager)
    {
        Manager->RemoveFromRoot();                  // remove UnLuaManager from root
        Manager = nullptr;
        SManager = nullptr;
    }

    DestroyDefaultParamCollection();                // destroy data of default parameters of UFunctions
}

/**
 * Callback for FCoreDelegates::OnAsyncLoadingFlushUpdate
 */
void FLuaContext::OnAsyncLoadingFlushUpdate()
{
    if (!Manager)
    {
        return;
    }

    static UClass *InterfaceClass = UUnLuaInterface::StaticClass();

    {
        FScopeLock Lock(&CandidatesCS);

        for (int32 i = Candidates.Num() - 1; i >= 0; --i)
        {
            UObject *Object = Candidates[i];
            if (Object && !Object->HasAnyFlags(RF_NeedPostLoad))
            {
                // see FLuaContext::TryToBindLua
                UFunction *Func = Object->FindFunction(FName("GetModuleName"));
                if (!Func || !Func->GetNativeFunc())
                {
                    continue;
                }
                FString ModuleName;
                Object->UObject::ProcessEvent(Func, &ModuleName);    // force to invoke UObject::ProcessEvent(...)
                UClass *Class = Func->GetOuterUClass();
                Class = Class == InterfaceClass ? Object->GetClass() : Class;
                if (ModuleName.Len() < 1)
                {
                    ModuleName = Class->GetName();
                }
                Manager->Bind(Object, Class, *ModuleName);
                Candidates.RemoveAt(i);
            }
        }
    }
}

/**
 * Callback for FCoreDelegates::OnHandleSystemError and FCoreDelegates::OnHandleSystemEnsure
 */
void FLuaContext::OnCrash()
{
    FString LogStr = UnLua::GetLuaCallStack(L);         // get lua call stack...

    UE_LOG(LogUnLua, Error, TEXT("%s"), *LogStr);

    GLog->Flush();
}

/**
 * Callback for FCoreUObjectDelegates::PreLoadMap
 */
void FLuaContext::PreLoadMap(const FString &MapName)
{
    int32 Loc = INDEX_NONE;
    MapName.FindLastChar('/', Loc);
    NextMap = Loc > INDEX_NONE ? MapName.Right(MapName.Len() - Loc - 1) : MapName;
    Initialize();
}

/**
 * Callback for FCoreUObjectDelegates::PostLoadMapWithWorld
 */
void FLuaContext::PostLoadMapWithWorld(UWorld *World)
{
    if (!World || !bEnable || !bInitialized || !Manager)
    {
        return;
    }

    static bool bGameInstanceBinded = false;
    UGameInstance *GameInstance = World->GetGameInstance();
    if (!bGameInstanceBinded && GameInstance)
    {
        TryToBindLua(GameInstance);                     // try to bind Lua module for GameInstance
        bGameInstanceBinded = true;
    }

    Manager->OnMapLoaded(World);

#if WITH_EDITOR
    LoadedWorlds.Add(World);
#endif

    // register callback for spawning an actor
    OnActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(Manager, &UUnLuaManager::OnActorSpawned));

#if UE_BUILD_TEST
    RunPerformanceTest(World);
#endif
}

/**
 * Callback for FCoreUObjectDelegates::GetPostGarbageCollect()
 */
void FLuaContext::OnPostGarbageCollect()
{
    if (L)
    {
        // check memory leaks under DEBUG build
#if UE_BUILD_DEBUG
        lua_getfield(L, LUA_REGISTRYINDEX, "StructMap");
        int32 N = TraverseTable(L, -1, nullptr, PeekTableElement);
        lua_pop(L, 1);
        if (N > 0)
        {
            UE_LOG(LogUnLua, Warning, TEXT("!!! %d structs are still alive !!!"), N);
        }
        lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");
        N = TraverseTable(L, -1, nullptr, PeekTableElement);
        lua_pop(L, 1);
        if (N > 0)
        {
            UE_LOG(LogUnLua, Warning, TEXT("!!! %d objects are still alive !!!"), N);
        }
#endif

        Manager->PostCleanup();
    }

    FCoreUObjectDelegates::GetPostGarbageCollect().Remove(OnPostGarbageCollectHandle);

    if (bIsInSeamlessTravel)
    {
        bIsInSeamlessTravel = false;
        Initialize();
    }
}

#if WITH_EDITOR
/**
 * Callback for FEditorDelegates::PreBeginPIE
 */
void FLuaContext::PreBeginPIE(bool bIsSimulating)
{
    bIsPIE = true;
#if AUTO_UNLUA_STARTUP
    SetEnable(true);
#else
    Initialize();
#endif
}

/**
 * Callback for FEditorDelegates::BeginPIE
 */
void FLuaContext::BeginPIE(bool bIsSimulating)
{
}

/**
 * Callback for FEditorDelegates::PostPIEStarted
 */
void FLuaContext::PostPIEStarted(bool bIsSimulating)
{
    if (!Manager)
    {
        return;
    }

    Manager->GetDefaultInputs();

    UEditorEngine *EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine)
    {
        //Manager->OnMapLoaded(EditorEngine->PlayWorld);
        UWorld *World = ServerWorld ? ServerWorld : EditorEngine->PlayWorld;
        if (World)
        {
            UGameInstance *GameInstance = World->GetGameInstance();
            TryToBindLua(GameInstance);

            Manager->OnMapLoaded(World);

            LoadedWorlds.Add(World);
        }
    }
}

/**
 * Callback for FEditorDelegates::PrePIEEnded
 */
void FLuaContext::PrePIEEnded(bool bIsSimulating)
{
    //bIsPIE = false;
}

/**
 * Callback for FEditorDelegates::EndPIE
 */
void FLuaContext::EndPIE(bool bIsSimulating)
{
}

/**
 * Callback for FGameDelegates::EndPlayMapDelegate
 */
void FLuaContext::OnEndPlayMap()
{
    bIsPIE = false;
    Cleanup(true);
    Manager->CleanupDefaultInputs();
    ServerWorld = nullptr;
    LoadedWorlds.Empty();
    CandidateInputComponents.Empty();
    FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
}
#endif

/**
 * Add a Lua coroutine and its reference in Lua registry
 */
void FLuaContext::AddThread(lua_State *Thread, int32 ThreadRef)
{
    ThreadToRef.Add(Thread, ThreadRef);
    RefToThread.Add(ThreadRef, Thread);
}

/**
 * Starts and resumes a Lua coroutine
 */
void FLuaContext::ResumeThread(int32 ThreadRef)
{
    lua_State **ThreadPtr = RefToThread.Find(ThreadRef);
    if (ThreadPtr)
    {
        lua_State *Thread = *ThreadPtr;
        int32 State = lua_resume(Thread, L, 0);
        if (State == LUA_OK)
        {
            ThreadToRef.Remove(Thread);
            RefToThread.Remove(ThreadRef);
            luaL_unref(L, LUA_REGISTRYINDEX, ThreadRef);    // remove the reference if the coroutine finishes its execution
        }
    }
}

/**
 * Clean up all Lua coroutines
 */
void FLuaContext::CleanupThreads()
{
    for (TMap<lua_State*, int32>::TIterator It(ThreadToRef); It; ++It)
    {
        lua_State *Thread = It.Key();
        int32 ThreadRef = It.Value();
        if (ThreadRef != LUA_REFNIL)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, ThreadRef);
        }
    }
    ThreadToRef.Empty();
    RefToThread.Empty();
}

/**
 * Find a Lua coroutine
 */
int32 FLuaContext::FindThread(lua_State *Thread)
{
    int32 *ThreadRefPtr = ThreadToRef.Find(Thread);
    return ThreadRefPtr ? *ThreadRefPtr : LUA_REFNIL;
}

/**
 * Callback when a UObjectBase (not full UObject) is created
 */
void FLuaContext::NotifyUObjectCreated(const UObjectBase *InObject, int32 Index)
{
    UObjectBaseUtility *Object = (UObjectBaseUtility*)InObject;
    TryToBindLua(Object);               // try to bind a Lua module for the object

    // special handling for UInputComponent
    if (!Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && Object->IsA<UInputComponent>())
    {
        AActor *Actor = Cast<APlayerController>(Object->GetOuter());
        if (!Actor)
        {
            Actor = Cast<APawn>(Object->GetOuter());
        }
        if (Actor && Actor->GetLocalRole() >= ROLE_AutonomousProxy)
        {
            CandidateInputComponents.AddUnique((UInputComponent*)InObject);
            if (!FWorldDelegates::OnWorldTickStart.IsBoundToObject(this))
            {
                OnWorldTickStartHandle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FLuaContext::OnWorldTickStart);
            }
        }
    }
}

/**
 * Callback when a UObjectBase (not full UObject) is deleted
 */
void FLuaContext::NotifyUObjectDeleted(const UObjectBase *InObject, int32 Index)
{
    if (!bEnable || !Manager)
    {
        return;
    }

    bool bClass = GReflectionRegistry.NotifyUObjectDeleted(InObject);
    Manager->NotifyUObjectDeleted(InObject, bClass);

    if (CandidateInputComponents.Num() > 0)
    {
        int32 NumRemoved = CandidateInputComponents.Remove((UInputComponent*)InObject);
        if (NumRemoved > 0 && CandidateInputComponents.Num() < 1)
        {
            FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
        }
    }
}

FLuaContext::FLuaContext()
    : L(nullptr), Manager(nullptr), bEnable(false), bInitialized(false), bIsPIE(false), bAddUObjectNotify(false), bDelegatesRegistered(false), bIsInSeamlessTravel(false)
{
#if WITH_EDITOR
    ServerWorld = nullptr;
    LuaHandle = nullptr;
#endif
}

FLuaContext::~FLuaContext()
{
    Cleanup(true);

    if (Manager)
    {
        Manager->RemoveFromRoot();
        Manager = nullptr;
    }
}

/**
 * Allocator for Lua VM
 */
void* FLuaContext::LuaAllocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
#if STATS
        const uint32 Size = FMemory::GetAllocSize(ptr);
        DEC_MEMORY_STAT_BY(STAT_UnLua_Lua_Memory, Size);
#endif
        FMemory::Free(ptr);
        return nullptr;
    }

    void *Buffer = nullptr;
    if (!ptr)
    {
        Buffer = FMemory::Malloc(nsize);
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        INC_MEMORY_STAT_BY(STAT_UnLua_Lua_Memory, Size);
#endif
    }
    else
    {
#if STATS
        const uint32 OldSize = FMemory::GetAllocSize(ptr);
#endif
        Buffer = FMemory::Realloc(ptr, nsize);
#if STATS
        const uint32 NewSize = FMemory::GetAllocSize(Buffer);
        if (NewSize > OldSize)
        {
            INC_MEMORY_STAT_BY(STAT_UnLua_Lua_Memory, NewSize - OldSize);
        }
        else
        {
            DEC_MEMORY_STAT_BY(STAT_UnLua_Lua_Memory, OldSize - NewSize);
        }
#endif
    }
    return Buffer;
}

/**
 * Initialize UnLua
 */
void FLuaContext::Initialize()
{
    if (!bEnable || bInitialized)
    {
        return;
    }

    FCollisionHelper::Initialize();     // initialize collision helper stuff

    CreateState();                      // create Lua main thread

    if (L)
    {
        if (!bAddUObjectNotify)
        {
            GUObjectArray.AddUObjectCreateListener(GLuaCxt);    // add listener for creating UObject
            GUObjectArray.AddUObjectDeleteListener(GLuaCxt);    // add listener for deleting UObject
            bAddUObjectNotify = true;
        }

        bInitialized = true;

        FUnLuaDelegates::OnLuaContextInitialized.Broadcast();
    }
}

/**
 * Clean up UnLua
 */
void FLuaContext::Cleanup(bool bFullCleanup, UWorld *World)
{
    if (!bEnable || !Manager)
    {
        return;
    }

    if (L)
    {
        FUnLuaDelegates::OnPreLuaContextCleanup.Broadcast(bFullCleanup);

        CleanupThreads();                                       // clean up coroutines

        FDelegateHelper::Cleanup(bFullCleanup);                 // clean up delegates

        Manager->Cleanup(World, bFullCleanup);                  // clean up UnLuaManager

        GPropertyCreator.Cleanup();                             // clean up dynamically created UProperties

        for (const FString &Name : LibraryNames)
        {
            ClearLibrary(L, TCHAR_TO_ANSI(*Name));              // clean up Lua meta tables
        }
        LibraryNames.Empty();

        for (const FString &Name : ModuleNames)
        {
            ClearLoadedModule(L, TCHAR_TO_ANSI(*Name));         // clean up required Lua modules
        }
        ModuleNames.Empty();

        if (bFullCleanup)
        {
            lua_close(L);
            L = nullptr;

            GObjectReferencer.Cleanup();                        // clean up object referencer
            GReflectionRegistry.Cleanup();                      // clean up reflection registry

#if WITH_EDITOR
            if (LuaHandle)
            {
                FPlatformProcess::FreeDllHandle(LuaHandle);     // unload Lua dynamic lib
                LuaHandle = nullptr;
            }
#endif

            FCollisionHelper::Cleanup();                        // clean up collision helper stuff

            if (bAddUObjectNotify)
            {
                // remove listeners for creating/deleting UObject
                GUObjectArray.RemoveUObjectCreateListener(GLuaCxt);
                GUObjectArray.RemoveUObjectDeleteListener(GLuaCxt);
                bAddUObjectNotify = false;
            }

            bEnable = false;
        }
        else
        {
            lua_gc(L, LUA_GCCOLLECT, 0);

            OnPostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FLuaContext::OnPostGarbageCollect);

#if UE_BUILD_DEBUG
            GObjectReferencer.Debug();
            GReflectionRegistry.Debug();
#endif
        }

        bInitialized = false;

        FUnLuaDelegates::OnPostLuaContextCleanup.Broadcast(bFullCleanup);
    }
}

/**
 * Build-in input event for 'Hotfix'
 */
bool FLuaContext::OnGameViewportInputKey(FKey InKey, FModifierKeysState ModifierKeyState, EInputEvent EventType)
{
    if (InKey == EKeys::L && ModifierKeyState.IsControlDown() && EventType == IE_Released)
    {
        return HotfixLua();
    }
    return false;
}
