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

#pragma once

#include "Framework/Commands/Commands.h"

class FUnLuaEditorCommands : public TCommands<FUnLuaEditorCommands>
{
public:
    FUnLuaEditorCommands()
        : TCommands<FUnLuaEditorCommands>(TEXT("UnLuaEditor"), NSLOCTEXT("Contexts", "UnLuaEditor", "UnLua Editor"), NAME_None, "UnLuaEditorStyle")
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> CreateLuaTemplate;
    TSharedPtr<FUICommandInfo> CopyAsRelativePath;
    TSharedPtr<FUICommandInfo> BindToLua;
    TSharedPtr<FUICommandInfo> UnbindFromLua;
    TSharedPtr<FUICommandInfo> HotReload;
    TSharedPtr<FUICommandInfo> OpenRuntimeSettings;
    TSharedPtr<FUICommandInfo> OpenEditorSettings;
    TSharedPtr<FUICommandInfo> ReportIssue;
    TSharedPtr<FUICommandInfo> About;
    TSharedPtr<FUICommandInfo> GenerateIntelliSense;
    TSharedPtr<FUICommandInfo> RevealInExplorer;
};
