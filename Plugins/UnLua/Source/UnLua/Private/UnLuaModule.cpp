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
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleManager.h"
#endif

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#include "UnLuaConsoleCommands.h"
#endif

#include "Engine/World.h"
#include "UnLuaModule.h"
#include "DefaultParamCollection.h"
#include "LuaEnvLocator.h"
#include "UnLuaDebugBase.h"
#include "UnLuaSettings.h"
#include "GameFramework/PlayerController.h"
#include "Registries/ClassRegistry.h"
#include "Registries/EnumRegistry.h"

#define LOCTEXT_NAMESPACE "FUnLuaModule"

namespace UnLua
{
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
            RegisterSettings();

#if ALLOW_CONSOLE
            ConsoleCommands = MakeUnique<FUnLuaConsoleCommands>(this);
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
            if (IsRunningGame() || IsRunningDedicatedServer())
#endif
                SetActive(true);
#endif
        }

        virtual void ShutdownModule() override
        {
            UnregisterSettings();
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

                const auto& Settings = *GetMutableDefault<UUnLuaSettings>();
                const auto EnvLocatorClass = *Settings.EnvLocatorClass == nullptr ? ULuaEnvLocator::StaticClass() : *Settings.EnvLocatorClass;
                EnvLocator = NewObject<ULuaEnvLocator>(GetTransientPackage(), EnvLocatorClass);
                EnvLocator->AddToRoot();
            }
            else
            {
                FCoreDelegates::OnHandleSystemError.Remove(OnHandleSystemErrorHandle);
                FCoreDelegates::OnHandleSystemEnsure.Remove(OnHandleSystemEnsureHandle);
                GUObjectArray.RemoveUObjectCreateListener(this);
                GUObjectArray.RemoveUObjectDeleteListener(this);
                EnvLocator->Reset();
                EnvLocator->RemoveFromRoot();
                EnvLocator = nullptr;
                FClassRegistry::Cleanup();
                FEnumRegistry::Cleanup();
                GPropertyCreator.Cleanup();
            }

            bIsActive = bActive;
        }

        virtual TSharedPtr<FLuaEnv> GetEnv(UObject* Object) override
        {
            if (!bIsActive)
                return nullptr;
            return EnvLocator->Locate(Object);
        }

        virtual void HotReload() override
        {
            if (!bIsActive)
                return;
            EnvLocator->HotReload();
        }

    private:
        virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override
        {
            // UE_LOG(LogTemp, Log, TEXT("NotifyUObjectCreated : %p"), ObjectBase);
            if (!bIsActive)
                return;

            UObject* Object = (UObject*)ObjectBase;

            const auto Env = EnvLocator->Locate(Object);
            // UE_LOG(LogTemp, Log, TEXT("Locate %s for %s"), *Env->GetName(), *ObjectBase->GetFName().ToString());
            Env->TryBind(Object);
            Env->TryReplaceInputs(Object);
        }

        virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override
        {
            // UE_LOG(LogTemp, Log, TEXT("NotifyUObjectDeleted : %p"), Object);
            if (!bIsActive)
                return;

            if (FClassRegistry::StaticUnregister(Object))
                return;

            FEnumRegistry::StaticUnregister(Object);
        }

        virtual void OnUObjectArrayShutdown() override
        {
            if (!bIsActive)
                return;

            GUObjectArray.RemoveUObjectCreateListener(this);
            GUObjectArray.RemoveUObjectDeleteListener(this);

            bIsActive = false;
        }

        void OnSystemError() const
        {
        }

#if WITH_EDITOR

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

#endif

        void RegisterSettings()
        {
#if WITH_EDITOR
            ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
            if (!SettingsModule)
                return;

            const auto Section = SettingsModule->RegisterSettings("Project", "Plugins", "UnLua",
                                                                  LOCTEXT("UnLuaEditorSettingsName", "UnLua"),
                                                                  LOCTEXT("UnLuaEditorSettingsDescription", "UnLua Runtime Settings"),
                                                                  GetMutableDefault<UUnLuaSettings>());
            Section->OnModified().BindRaw(this, &FUnLuaModule::OnSettingsModified);
#endif
        }

        void UnregisterSettings()
        {
#if WITH_EDITOR
            ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
            if (SettingsModule)
                SettingsModule->UnregisterSettings("Project", "Plugins", "UnLua");
#endif
        }

        bool OnSettingsModified()
        {
            return true;
        }

        void PostLoadMapWithWorld(UWorld* World) const
        {
            if (!World || !bIsActive)
                return;

            const auto Env = EnvLocator->Locate(World);
            if (!Env)
                return;

            const auto Manager = Env->GetManager();
            if (!Manager)
                return;

            Manager->OnMapLoaded(World);
        }

        bool bIsActive = false;
        ULuaEnvLocator* EnvLocator = nullptr;
        FDelegateHandle OnHandleSystemErrorHandle;
        FDelegateHandle OnHandleSystemEnsureHandle;
#if ALLOW_CONSOLE
        TUniquePtr<UnLua::FUnLuaConsoleCommands> ConsoleCommands;
#endif
    };
}

IMPLEMENT_MODULE(UnLua::FUnLuaModule, UnLua)

#undef LOCTEXT_NAMESPACE
