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

using System.IO;
using UnrealBuildTool;

public class UnLua : ModuleRules
{
	public UnLua(ReadOnlyTargetRules Target) : base(Target)
    {
        SetupScripts();

        bEnforceIWYU = false;

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
            }
			);


		PrivateIncludePaths.AddRange(
			new string[] {
                "UnLua/Private",
            }
            );


        PublicIncludePathModuleNames.AddRange(
            new string[] {
                "ApplicationCore",
            }
        );


        PrivateDependencyModuleNames.AddRange(
			new string[]
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
		
        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        bool bSupportCommandlet = true;
        if (bSupportCommandlet)
        {
            PublicDefinitions.Add("SUPPORT_COMMANDLET=1");
        }
        else
        {
            PublicDefinitions.Add("SUPPORT_COMMANDLET=0");
        }

        bool bAutoStartup = true;
        if (bAutoStartup)
        {
            PublicDefinitions.Add("AUTO_UNLUA_STARTUP=1");
        }
        else
        {
            PublicDefinitions.Add("AUTO_UNLUA_STARTUP=0");
        }

        bool bWithUE4Namespace = true;
        if (bWithUE4Namespace)
        {
            PublicDefinitions.Add("WITH_UE4_NAMESPACE=1");
        }
        else
        {
            PublicDefinitions.Add("WITH_UE4_NAMESPACE=0");
        }
    
        bool bSupportsRpcCall = true;
        if (bSupportsRpcCall)
        {
            PublicDefinitions.Add("SUPPORTS_RPC_CALL=1");
        }
        else
        {
            PublicDefinitions.Add("SUPPORTS_RPC_CALL=0");
        }

        bool bSupportsCommandlet = true;
        if (bSupportsCommandlet)
        {
            PublicDefinitions.Add("SUPPORTS_COMMANDLET=1");
        }
        else
        {
            PublicDefinitions.Add("SUPPORTS_COMMANDLET=0");
        }

        bool bEnableAutoCleanNoneNativeClass = true;
        if (bEnableAutoCleanNoneNativeClass)
        {
            PublicDefinitions.Add("ENABLE_AUTO_CLEAN_NNATCLASS=1");
        }
        else
        {
            PublicDefinitions.Add("ENABLE_AUTO_CLEAN_NNATCLASS=0");
        }

        bool bEnableTypeCheck = true;
        if (bEnableTypeCheck)
        {
            PublicDefinitions.Add("ENABLE_TYPE_CHECK=1");
        }
        else
        {
            PublicDefinitions.Add("ENABLE_TYPE_CHECK=0");
        }

        bool bEnableDebug = false;
        if (bEnableDebug)
        {
            PublicDefinitions.Add("UNLUA_ENABLE_DEBUG=1");
        }
        else
        {
            PublicDefinitions.Add("UNLUA_ENABLE_DEBUG=0");
        }

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
        if (!File.Exists(DstPath))
            File.Copy(SrcPath, DstPath);
    }
}
