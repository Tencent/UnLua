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

#include "UnLuaPrivate.h"
#include "UnLuaEditorStyle.h"
#include "UnLuaEditorCommands.h"
#include "Misc/CoreDelegates.h"
#include "Editor.h"
#include "BlueprintEditorModule.h"
#include "UnLuaIntelliSenseGenerator.h"
#include "UnLua.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "UnLuaEditorSettings.h"
#include "UnLuaEditorFunctionLibrary.h"
#include "UnLuaFunctionLibrary.h"
#include "Kismet2/DebuggerCommands.h"
#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"
#include "Toolbars/AnimationBlueprintToolbar.h"
#include "Toolbars/BlueprintToolbar.h"
#include "Toolbars/MainMenuToolbar.h"

#define LOCTEXT_NAMESPACE "FUnLuaEditorModule"

// copy dependency file to plugin's content dir
static bool CopyDependencyFile(const TCHAR* FileName)
{
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();
    FString SrcFilePath = ContentDir / FileName;
    FString DestFilePath = GLuaSrcFullPath / FileName;
    bool bSuccess = IFileManager::Get().FileExists(*DestFilePath);
    if (!bSuccess)
    {
        bSuccess = IFileManager::Get().FileExists(*SrcFilePath);
        if (!bSuccess)
        {
            return false;
        }

        uint32 CopyResult = IFileManager::Get().Copy(*DestFilePath, *SrcFilePath, 1, true);
        if (CopyResult != COPY_OK)
        {
            return false;
        }
    }
    return true;
}

class FUnLuaEditorModule : public IModuleInterface
{
public:
    FUnLuaEditorModule()
    {
    }

    virtual void StartupModule() override
    {
        Style = FUnLuaEditorStyle::GetInstance();

        FUnLuaEditorCommands::Register();

        OnPostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnLuaEditorModule::OnPostEngineInit);

        MainMenuToolbar = MakeShareable(new FMainMenuToolbar);
        BlueprintToolbar = MakeShareable(new FBlueprintToolbar);
        AnimationBlueprintToolbar = MakeShareable(new FAnimationBlueprintToolbar);

        UUnLuaEditorFunctionLibrary::WatchScriptDirectory();
    }

    virtual void ShutdownModule() override
    {
        FUnLuaEditorCommands::Unregister();
        FCoreDelegates::OnPostEngineInit.Remove(OnPostEngineInitHandle);
        UnregisterSettings();
    }

private:
    void OnPostEngineInit()
    {
        RegisterSettings();

        if (!GEditor)
        {
            // Loading MainFrame module with '-game' is not supported
            return;
        }

        MainMenuToolbar->Initialize();
        BlueprintToolbar->Initialize();
        AnimationBlueprintToolbar->Initialize();
        FUnLuaIntelliSenseGenerator::Get()->Initialize();

        IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
        MainFrameModule.OnMainFrameCreationFinished().AddRaw(this, &FUnLuaEditorModule::OnMainFrameCreationFinished);
    }

    void OnMainFrameCreationFinished(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow)
    {
        // register default key input to 'HotReload' Lua
        FPlayWorldCommands::GlobalPlayWorldActions->MapAction(
            FUnLuaEditorCommands::Get().HotReload, FExecuteAction::CreateStatic(UUnLuaFunctionLibrary::HotReload), FCanExecuteAction());

        // copy dependency files
        bool bSuccess = CopyDependencyFile(TEXT("UnLua.lua"));
        check(bSuccess);
        CopyDependencyFile(TEXT("UnLuaPerformanceTestProxy.lua"));
        check(bSuccess);
    }

    void RegisterSettings() const
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
        if (SettingsModule)
        {
            const TSharedPtr<ISettingsSection> Section = SettingsModule->RegisterSettings("Project", "Plugins", "UnLua Editor",
                                                                                          LOCTEXT("UnLuaEditorSettingsName", "UnLua Editor"),
                                                                                          LOCTEXT("UnLuaEditorSettingsDescription", "UnLua Editor Settings"),
                                                                                          GetMutableDefault<UUnLuaEditorSettings>());
            Section->OnModified().BindRaw(this, &FUnLuaEditorModule::OnSettingsModified);
        }
    }

    void UnregisterSettings() const
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
        if (SettingsModule)
            SettingsModule->UnregisterSettings("Project", "Plugins", "UnLua Editor");
    }

    bool OnSettingsModified() const
    {
        const auto BuildFile = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetBaseDir() / "Source/UnLua/UnLua.Build.cs";
        auto& FileManager = IFileManager::Get();
        if (FileManager.FileExists(*BuildFile))
            FileManager.SetTimeStamp(*BuildFile, FDateTime::UtcNow());
        return true;
    }

    TSharedPtr<FBlueprintToolbar> BlueprintToolbar;
    TSharedPtr<FAnimationBlueprintToolbar> AnimationBlueprintToolbar;
    TSharedPtr<FMainMenuToolbar> MainMenuToolbar;
    TSharedPtr<FUnLuaIntelliSenseGenerator> IntelliSenseGenerator;
    TSharedPtr<ISlateStyle> Style;

    FDelegateHandle OnPostEngineInitHandle;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaEditorModule, UnLuaEditor)
