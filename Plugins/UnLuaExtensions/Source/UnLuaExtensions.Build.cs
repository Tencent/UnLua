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

using System;
using System.IO;
using UnrealBuildTool;

public class UnLuaExtensions : ModuleRules
{
    public UnLuaExtensions(ReadOnlyTargetRules Target) : base(Target)
    {
        bEnforceIWYU = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        bEnableUndefinedIdentifierWarnings = false;
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
            });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnLua",
            "Lua",
        });

        PrivateDefinitions.AddRange(
            new[]
            {
                "EXTENSION_NAMESPACE_BEGIN=namespace UnLuaExtensions { namespace LuaSocket {",
                "EXTENSION_NAMESPACE_END=}}"
            }
        );

        var BasePath = Path.GetFullPath(Path.Combine(ModuleDirectory, "ThirdParty", "LuaSocket"));
        PublicIncludePaths.Add(BasePath);
    }
}