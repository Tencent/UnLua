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
#include "IPersonaToolkit.h"
#include "Textures/SlateIcon.h"
#include "Editor.h"
#include "Animation/AnimBlueprint.h"
#include "BlueprintEditorModule.h"
#include "AnimationBlueprintEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/DebuggerCommands.h"
#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"
#include "Toolbars/AnimationBlueprintToolbar.h"
#include "Toolbars/BlueprintToolbar.h"

#define LOCTEXT_NAMESPACE "FUnLuaEditorModule"

// copy dependency file to plugin's content dir
static bool CopyDependencyFile(const TCHAR *FileName)
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
        : bIsPIE(false)
    {
    }

    virtual void StartupModule() override
    {
        Style = MakeShareable(new FUnLuaEditorStyle);

        FUnLuaEditorCommands::Register();

        // register delegates
        OnPostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnLuaEditorModule::OnPostEngineInit);
        PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FUnLuaEditorModule::PostPIEStarted);
        PrePIEEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FUnLuaEditorModule::PrePIEEnded);

        BlueprintToolbar = MakeShareable(new FBlueprintToolbar);
        AnimationBlueprintToolbar = MakeShareable(new FAnimationBlueprintToolbar);
    }

    virtual void ShutdownModule() override
    {
        FUnLuaEditorCommands::Unregister();

        // unregister delegates
        FCoreDelegates::OnPostEngineInit.Remove(OnPostEngineInitHandle);
        FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
        FEditorDelegates::PrePIEEnded.Remove(PrePIEEndedHandle);
    }

private:
    void OnPostEngineInit()
    {
        BlueprintToolbar->Initialize();
        AnimationBlueprintToolbar->Initialize();
        
        IMainFrameModule &MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
        MainFrameModule.OnMainFrameCreationFinished().AddRaw(this, &FUnLuaEditorModule::OnMainFrameCreationFinished);
    }

    void PostPIEStarted(bool bIsSimulating)
    {
        bIsPIE = true;      // raise PIE flag
    }

    void PrePIEEnded(bool bIsSimulating)
    {
        bIsPIE = false;     // clear PIE flag
    }

    void OnMainFrameCreationFinished(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow)
    {
        // register default key input to 'Hotfix' Lua
        FPlayWorldCommands::GlobalPlayWorldActions->MapAction(FUnLuaEditorCommands::Get().HotfixLua, FExecuteAction::CreateLambda([]() { HotfixLua(); }), FCanExecuteAction::CreateRaw(this, &FUnLuaEditorModule::CanHotfixLua));

        // copy dependency files
        bool bSuccess = CopyDependencyFile(TEXT("UnLua.lua"));
        check(bSuccess);
        CopyDependencyFile(TEXT("UnLuaPerformanceTestProxy.lua"));
        check(bSuccess);
    }

    bool CanHotfixLua() const
    {
        return bIsPIE;      // enable Lua 'Hotfix' in PIE
    }

    TSharedPtr<FBlueprintToolbar> BlueprintToolbar;
    TSharedPtr<FAnimationBlueprintToolbar> AnimationBlueprintToolbar;
    TSharedPtr<ISlateStyle> Style;

    FDelegateHandle OnPostEngineInitHandle;
    FDelegateHandle PostPIEStartedHandle;
    FDelegateHandle PrePIEEndedHandle;

    bool bIsPIE;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaEditorModule, UnLuaEditor)
