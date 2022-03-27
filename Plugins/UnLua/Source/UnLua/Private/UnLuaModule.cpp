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

#if WITH_EDITOR
#include "Modules/ModuleManager.h"
#endif

#include "UnLuaModule.h"

#include "DefaultParamCollection.h"
#include "UnLuaDebugBase.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "FUnLuaModule"


class FUnLuaModule : public IUnLuaModule,
                     public FUObjectArray::FUObjectCreateListener,
                     public FUObjectArray::FUObjectDeleteListener
{
public:
    virtual void StartupModule() override
    {
#if WITH_EDITOR
        FModuleManager::Get().LoadModule(TEXT("UnLuaEditor"));
#endif

        FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FUnLuaModule::PostLoadMapWithWorld);

        CreateDefaultParamCollection();

#if WITH_EDITOR
        if (!IsRunningGame())
        {
            FEditorDelegates::PreBeginPIE.AddRaw(this, &FUnLuaModule::OnPreBeginPIE);
            FEditorDelegates::PostPIEStarted.AddRaw(this, &FUnLuaModule::OnPostPIEStarted);
            FEditorDelegates::EndPIE.AddRaw(this, &FUnLuaModule::OnEndPIE);
        }
#endif

#if AUTO_UNLUA_STARTUP
#if WITH_EDITOR
        if (IsRunningGame())
#endif
            SetActive(true);
#endif
    }

    virtual void ShutdownModule() override
    {
        SetActive(false);
    }

    virtual bool IsActive() override
    {
        return bIsActive;
    }

    virtual void SetActive(const bool bActive) override
    {
        if (bIsActive == bActive)
            return;

        if (bActive)
        {
            OnHandleSystemErrorHandle = FCoreDelegates::OnHandleSystemError.AddRaw(this, &FUnLuaModule::OnSystemError);
            OnHandleSystemEnsureHandle = FCoreDelegates::OnHandleSystemEnsure.AddRaw(this, &FUnLuaModule::OnSystemError);
            GUObjectArray.AddUObjectCreateListener(this);
            GUObjectArray.AddUObjectDeleteListener(this);
            Env = MakeShared<UnLua::FLuaEnv>();
        }
        else
        {
            FCoreDelegates::OnHandleSystemError.Remove(OnHandleSystemErrorHandle);
            FCoreDelegates::OnHandleSystemEnsure.Remove(OnHandleSystemEnsureHandle);
            GUObjectArray.RemoveUObjectCreateListener(this);
            GUObjectArray.RemoveUObjectDeleteListener(this);
            Env.Reset();
        }

        bIsActive = bActive;
    }

    virtual TSharedPtr<UnLua::FLuaEnv> GetEnv() override
    {
        return Env;
    }

    virtual void HotReload() override
    {
        if (Env)
            Env->HotReload();
    }

private:
    virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override
    {
        if (!bIsActive)
            return;

        UObject* Object = (UObject*)ObjectBase;
        Env->TryBind(Object);
        Env->TryReplaceInputs(Object);
    }

    virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override
    {
        if (!bIsActive)
            return;
    }

    virtual void OnUObjectArrayShutdown() override
    {
        if (!bIsActive)
            return;

        GUObjectArray.RemoveUObjectCreateListener(this);
        GUObjectArray.RemoveUObjectDeleteListener(this);

        bIsActive = false;
    }

    void OnSystemError()
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

    void OnPreBeginPIE(bool bIsSimulating)
    {
        SetActive(true);
    }

    void OnPostPIEStarted(bool bIsSimulating)
    {
        UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
        if (EditorEngine)
            PostLoadMapWithWorld(EditorEngine->PlayWorld);
    }

    void OnEndPIE(bool bIsSimulating)
    {
        SetActive(false);
    }

    void PostLoadMapWithWorld(UWorld* World) const
    {
        if (!World || !bIsActive || !Env)
            return;

        const auto Manager = Env->GetManager();
        if (!Manager)
            return;

        Manager->OnMapLoaded(World);
    }

    bool bIsActive;
    TSharedPtr<UnLua::FLuaEnv> Env;
    FDelegateHandle OnHandleSystemErrorHandle;
    FDelegateHandle OnHandleSystemEnsureHandle;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaModule, UnLua)
