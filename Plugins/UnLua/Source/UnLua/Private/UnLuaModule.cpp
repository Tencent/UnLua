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

#include "LuaContext.h"
#include "UnLuaSettings.h"

#define LOCTEXT_NAMESPACE "FUnLuaModule"

class FUnLuaModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
#if WITH_EDITOR
        FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this] { RegisterSettings(); });
#endif
        FLuaContext::Create();
        GLuaCxt->RegisterDelegates();
    }

    virtual void ShutdownModule() override
    {
#if WITH_EDITOR
        UnregisterSettings();
#endif
    }

private:
#if WITH_EDITOR
    void RegisterSettings() const
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
        if (SettingsModule)
        {
            const TSharedPtr<ISettingsSection> Section = SettingsModule->RegisterSettings("Project", "Plugins", "UnLua",
                                                                                          LOCTEXT("UnLuaSettingsName", "UnLua"),
                                                                                          LOCTEXT("UnLuaSettingsDescription", "Configure settings of UnLua."),
                                                                                          GetMutableDefault<UUnLuaSettings>());
            Section->OnModified().BindRaw(this, &FUnLuaModule::OnSettingsModified);
        }
    }

    void UnregisterSettings() const
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
        if (SettingsModule)
            SettingsModule->UnregisterSettings("Project", "Plugins", "UnLua");
    }

    bool OnSettingsModified() const
    {
        const UUnLuaSettings& Settings = *GetDefault<UUnLuaSettings>();

        // TODO:

        return true;
    }
#endif
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaModule, UnLua)
