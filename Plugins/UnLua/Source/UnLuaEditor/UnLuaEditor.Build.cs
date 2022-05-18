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

using UnrealBuildTool;
using System.IO;

public class UnLuaEditor : ModuleRules
{
    public UnLuaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        bEnforceIWYU = false;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
            }
        );

        string EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
        PrivateIncludePaths.AddRange(
            new[]
            {
                "UnLuaEditor/Private",
                "UnLua/Private",
                Path.Combine(EngineDir, "Source/Editor/AnimationBlueprintEditor/Private"),
                Path.Combine(EngineDir, "Source/Runtime/Slate/Private"),
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new[]
            {
                "Kismet",
                "MainFrame",
                "AnimationBlueprintEditor",
                "Slate",
                "SlateCore",
                "Persona",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "EditorStyle",
                "ApplicationCore",
                "Projects",
                "InputCore",
                "UMG",
                "UMGEditor",
                "BlueprintGraph",
                "Slate",
                "SlateCore",
                "DirectoryWatcher",
                "Networking",
                "Sockets",
                "UnLua",
                "Lua",
                "ToolMenus"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new[]
            {
                "Kismet",
                "MainFrame",
                "AnimationBlueprintEditor",
            }
        );
    }
}