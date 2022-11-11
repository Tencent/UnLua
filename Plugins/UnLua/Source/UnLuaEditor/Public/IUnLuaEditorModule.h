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

#include "LuaEnv.h"

class UNLUAEDITOR_API IUnLuaEditorModule : public IModuleInterface
{
public:
    static FORCEINLINE IUnLuaEditorModule& Get()
    {
        return FModuleManager::LoadModuleChecked<IUnLuaEditorModule>("UnLuaEditor");
    }

    /** 获取编辑器专用的Lua环境 */
    virtual UnLua::FLuaEnv* GetEnv() = 0;

    /**
     * 增加一个目录到打包设置。
     * @param RelativePath 相对于"工程目录/Content"的路径，如：../Plugins/UnLua/Content/Script
     * @param bPak 是否使用pak方式打包
     */
    virtual void AddPackagingDirectory(FString& RelativePath, bool bPak) const = 0;
};
