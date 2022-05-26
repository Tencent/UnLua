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
#if UE_5_0_OR_LATER
using EpicGames.Core;
#else
using Tools.DotNETCommon;
#endif
using UnrealBuildTool;

public class UnLua : ModuleRules
{
    public UnLua(ReadOnlyTargetRules Target) : base(Target)
    {
        SetupScripts();

        bEnforceIWYU = false;
        bUseUnity = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
            }
        );

        PrivateIncludePaths.AddRange(
            new[]
            {
                "UnLua/Private",
            }
        );

        PublicIncludePathModuleNames.AddRange(
            new[]
            {
                "ApplicationCore",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "InputCore",
                "Projects",
                "Lua"
            }
        );

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.Add("UnrealEd");

        var projectDir = Target.ProjectFile.Directory;
        var configFilePath = projectDir + "/Config/DefaultUnLuaEditor.ini";
        var configFileReference = new FileReference(configFilePath); 
        var configFile = FileReference.Exists(configFileReference) ? new ConfigFile(configFileReference) : new ConfigFile();
        var config = new ConfigHierarchy(new[] { configFile });
        const string Section = "/Script/UnLuaEditor.UnLuaEditorSettings";

        Action<string, string, bool> loadBoolConfig = (key, macro, defaultValue) =>
        {
            bool flag;
            if (!config.GetBool(Section, key, out flag))
                flag = defaultValue;
            PublicDefinitions.Add(string.Format("{0}={1}", macro, (flag ? "1" : "0")));
        };

        loadBoolConfig("bAutoStartup", "AUTO_UNLUA_STARTUP", true);
        loadBoolConfig("bEnableDebug", "UNLUA_ENABLE_DEBUG", false);
        loadBoolConfig("bEnablePersistentParamBuffer", "ENABLE_PERSISTENT_PARAM_BUFFER", true);
        loadBoolConfig("bEnableTypeChecking", "ENABLE_TYPE_CHECK", true);
        loadBoolConfig("bEnableRPCCall", "SUPPORTS_RPC_CALL", true);
        loadBoolConfig("bEnableCallOverriddenFunction", "ENABLE_CALL_OVERRIDDEN_FUNCTION", true);
        loadBoolConfig("bWithUENamespace", "WITH_UE4_NAMESPACE", true);
        loadBoolConfig("bLegacyReturnOrder", "UNLUA_LEGACY_RETURN_ORDER", false);
        loadBoolConfig("bLegacyBlueprintPath", "UNLUA_LEGACY_BLUEPRINT_PATH", false);
    }

    private void SetupScripts()
    {
        const string UnLuaSourceFileName = "UnLua.lua";
        var PluginContentDirectory = Path.Combine(PluginDirectory, "Content");
        var DefaultScriptDirectory = Path.Combine(Target.ProjectFile.Directory.ToString(), "Content/Script");
        if (!Directory.Exists(DefaultScriptDirectory))
            Directory.CreateDirectory(DefaultScriptDirectory);

        var SrcPath = Path.Combine(PluginContentDirectory, UnLuaSourceFileName);
        var DstPath = Path.Combine(DefaultScriptDirectory, UnLuaSourceFileName);
        if (!File.Exists(DstPath) && File.Exists(SrcPath))
            File.Copy(SrcPath, DstPath);
    }
}