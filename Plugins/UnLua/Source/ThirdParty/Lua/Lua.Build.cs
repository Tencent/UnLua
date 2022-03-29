// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class Lua : ModuleRules
{
    public Lua(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = false;
        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

        bEnableUndefinedIdentifierWarnings = false;
        ShadowVariableWarningLevel = WarningLevel.Off;

#if UE_5_0_OR_LATER
        if (Target.Platform == UnrealTargetPlatform.Win64)
#else
        if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
#endif
        {
            PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
            if (Target.LinkType != TargetLinkType.Monolithic)
            {
                PublicDefinitions.Add("LUA_BUILD_AS_DLL");
            }
        }

        if (Target.Platform == UnrealTargetPlatform.Android || Target.Platform == UnrealTargetPlatform.Mac
             || Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Linux)
        {
            if (Target.LinkType != TargetLinkType.Monolithic)
            {
                PrivateDefinitions.Add("LUA_BUILD_AS_DLL");
            }
            PrivateDefinitions.Add("LUA_USE_DLOPEN");
        }

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "src"));
    }
}
