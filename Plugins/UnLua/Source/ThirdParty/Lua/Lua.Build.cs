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

public class Lua : ModuleRules
{
    public Lua(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac || 
            Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Android)
        {
            string LuaDynLibName = "";
            string LuaDynamicLibPath = "";

            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                LuaDynLibName = "Lua.dll";
                LuaDynamicLibPath = Path.Combine(ModuleDirectory, "binaries/Win64", LuaDynLibName);

                string Format = Path.Combine(ModuleDirectory, "{0}/Win64/Lua.lib");
                PublicAdditionalLibraries.Add(String.Format(Format, Target.bBuildEditor == true ? "binaries" : "lib"));
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                LuaDynLibName = Path.Combine(ModuleDirectory, "binaries/Mac/liblua.dylib");
                LuaDynamicLibPath = LuaDynLibName;

                if (!Target.bBuildEditor)
                {
                    PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/Mac/liblua.a"));
                }
            }
            else if (Target.Platform == UnrealTargetPlatform.IOS)
            {
                PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/IOS/liblua.a"));
            }
            else        // UnrealTargetPlatform.Android
            {
                //PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "lib/Android/ARMv7"));
                //PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "lib/Android/ARM64"));
                //PublicAdditionalLibraries.Add("lua");
                PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/Android/ARMv7/liblua.a"));
                PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/Android/ARM64/liblua.a"));
            }

            if (Target.bBuildEditor == true)
            {
                PublicDelayLoadDLLs.Add(LuaDynLibName);
                RuntimeDependencies.Add(LuaDynamicLibPath);
            }

            PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
        }
    }
}
