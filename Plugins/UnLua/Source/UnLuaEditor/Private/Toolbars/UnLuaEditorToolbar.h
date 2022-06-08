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

#include "BlueprintEditor.h"

class FUnLuaEditorToolbar
{
public:
    virtual ~FUnLuaEditorToolbar() = default;

    FUnLuaEditorToolbar();

    TSharedRef<FUICommandList> GetCommandList() const
    {
        return CommandList;
    }

    virtual void Initialize();

    void CreateLuaTemplate_Executed();

    void CopyAsRelativePath_Executed() const;

    void BindToLua_Executed() const;

    void UnbindFromLua_Executed() const;

    void RevealInExplorer_Executed();

protected:
    virtual void BindCommands();

    void BuildToolbar(FToolBarBuilder& ToolbarBuilder, UObject* InContextObject);

    void BuildNodeMenu();

    TSharedRef<FExtender> GetExtender(UObject* InContextObject);

    const TSharedRef<FUICommandList> CommandList;

    UObject* ContextObject;
};
