// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnLuaFrameWork : ModuleRules
{
	public UnLuaFrameWork(ReadOnlyTargetRules Target) : base(Target)
	{
        bEnforceIWYU = false;

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        ShadowVariableWarningLevel = WarningLevel.Off;
        bEnableUndefinedIdentifierWarnings = false;

        PublicIncludePaths.AddRange(
			new string[] {
            }
			);
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "UnLua/Private",
                "UnLuaFrameWork/Private",
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
                "Lua",
                "UnLua",
            }
			);
    }
}
