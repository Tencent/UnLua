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


#include "MainMenuToolbar.h"

#include "UnLuaIntelliSenseGenerator.h"
#include "LevelEditor.h"
#include "UnLuaEditorCommands.h"
#include "UnLuaFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "UnLuaMainMenuToolbar"

FMainMenuToolbar::FMainMenuToolbar()
    : CommandList(new FUICommandList)
{
    CommandList->MapAction(FUnLuaEditorCommands::Get().HotReload, FExecuteAction::CreateStatic(UUnLuaFunctionLibrary::HotReload), FCanExecuteAction());
    
    CommandList->MapAction(FUnLuaEditorCommands::Get().GenerateIntelliSense, FExecuteAction::CreateLambda([]
    {
        FUnLuaIntelliSenseGenerator::Get()->UpdateAll();
    }), FCanExecuteAction());

    CommandList->MapAction(FUnLuaEditorCommands::Get().ReportIssue, FExecuteAction::CreateLambda([]
    {
        const TCHAR* URL = TEXT("cmd");
        const TCHAR* Params = TEXT("/k start https://github.com/Tencent/UnLua/issues/new/choose");
        FPlatformProcess::ExecProcess(URL, Params, nullptr, nullptr, nullptr);
    }), FCanExecuteAction());

    CommandList->MapAction(FUnLuaEditorCommands::Get().About, FExecuteAction::CreateLambda([]
    {
        const TCHAR* URL = TEXT("cmd");
        const TCHAR* Params = TEXT("/k start https://github.com/Tencent/UnLua");
        FPlatformProcess::ExecProcess(URL, Params, nullptr, nullptr, nullptr);
    }), FCanExecuteAction());
}

void FMainMenuToolbar::Initialize()
{
    TSharedPtr<FExtender> Extender = MakeShareable(new FExtender);
    Extender->AddToolBarExtension("Settings", EExtensionHook::After, CommandList,
                                  FToolBarExtensionDelegate::CreateRaw(this, &FMainMenuToolbar::AddToolbarExtension)
    );

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(Extender);
}

void FMainMenuToolbar::AddToolbarExtension(FToolBarBuilder& Builder)
{
    Builder.BeginSection(NAME_None);


    Builder.AddComboButton(FUIAction(), FOnGetContent::CreateLambda([this]()
                           {
                               const FUnLuaEditorCommands& Commands = FUnLuaEditorCommands::Get();
                               FMenuBuilder MenuBuilder(true, CommandList);

                               MenuBuilder.BeginSection(NAME_None, LOCTEXT("Section_Action", "Action"));
                               MenuBuilder.AddMenuEntry(Commands.HotReload, NAME_None, LOCTEXT("HotReload", "Hot Reload"));
                               MenuBuilder.AddMenuEntry(Commands.GenerateIntelliSense, NAME_None, LOCTEXT("GenerateIntelliSense", "Generate IntelliSense"));
                               MenuBuilder.EndSection();

                               MenuBuilder.BeginSection(NAME_None, LOCTEXT("Section_Help", "Help"));
                               MenuBuilder.AddMenuEntry(Commands.ReportIssue, NAME_None, LOCTEXT("ReportIssue", "Report Issue"));
                               MenuBuilder.AddMenuEntry(Commands.About, NAME_None, LOCTEXT("About", "About"));
                               MenuBuilder.EndSection();

                               return MenuBuilder.MakeWidget();
                           }),
                           LOCTEXT("UnLua_Label", "UnLua"),
                           LOCTEXT("UnLua_ToolTip", "UnLua"),
                           FSlateIcon("UnLuaEditorStyle", "UnLuaEditor.UnLuaLogo")
    );

    Builder.EndSection();
}
