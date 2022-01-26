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

namespace UnrealBuildTool.Rules
{
    public class UnLuaIntelliSense : ModuleRules
    {
        public UnLuaIntelliSense(ReadOnlyTargetRules Target) : base(Target)
        {
            bEnforceIWYU = false;

            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new[]
                {
                    "Programs/UnrealHeaderTool/Public",
                }
            );


            PrivateIncludePaths.AddRange(
                new[]
                {
                    "UnLuaIntelliSense/Private",
                    "UnLua/Private",
                }
            );


            PrivateDependencyModuleNames.AddRange(
                new[]
                {
                    "Core",
                    "CoreUObject",
                }
            );

            var projectDir = Target.ProjectFile.Directory;
            var config = ConfigCache.ReadHierarchy(ConfigHierarchyType.Game, projectDir, Target.Platform);
            const string Section = "/Script/UnLuaEditor.UnLuaEditorSettings";

            Action<string, string, bool> loadBoolConfig = (key, macro, defaultValue) =>
            {
                bool flag;
                if (!config.GetBool(Section, key, out flag))
                    flag = defaultValue;
                PublicDefinitions.Add(string.Format("{0}={1}", macro, (flag ? "1" : "0")));
            };

            loadBoolConfig("bGenerateIntelliSense", "ENABLE_INTELLISENSE", true);

            PublicDefinitions.Add("HACK_HEADER_GENERATOR=1");
        }
    }
}