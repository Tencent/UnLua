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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
#if UE_5_0_OR_LATER
using EpicGames.Core;
#else
using Tools.DotNETCommon;
#endif
using UnrealBuildTool;

public class Lua : ModuleRules
{
    public Lua(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        bEnableUndefinedIdentifierWarnings = false;
        ShadowVariableWarningLevel = WarningLevel.Off;

        m_LuaVersion = "5.4.3";
        m_Config = GetConfigName();
        m_LibName = GetLibraryName();
        m_CompileAsCpp = ShouldCompileAsCpp();
        m_CompileAsDynamicLib = ShouldCompileAsDynamicLib();
        m_LibDirName = string.Format("lib-{0}", m_CompileAsCpp ? "cpp" : "c");
        m_LuaDirName = string.Format("lua-{0}", m_LuaVersion);
        m_LibraryPath = Path.Combine(ModuleDirectory, m_LuaDirName, m_LibDirName, Target.Platform.ToString(), m_Config, m_LibName);

        if (!File.Exists(m_LibraryPath))
        {
            GenerateLibrary();
            var buildLibPath = Path.Combine(ModuleDirectory, m_LuaDirName, "build", m_Config, m_LibName);
            EnsureDirectoryExists(m_LibraryPath);
            File.Copy(buildLibPath, m_LibraryPath);

            if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
            {
                // windows下dll需要额外的lib文件
                File.Copy(Path.ChangeExtension(buildLibPath, ".lib"), Path.ChangeExtension(m_LibraryPath, ".lib"), true);

                var buildPdbPath = Path.ChangeExtension(buildLibPath, ".pdb");
                if (File.Exists(buildPdbPath))
                {
                    var pdbPath = Path.ChangeExtension(m_LibraryPath, ".pdb");
                    File.Copy(buildPdbPath, pdbPath, true);
                }
            }
        }

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, m_LuaDirName, "src"));

        if (m_CompileAsDynamicLib)
        {
            SetupForRuntimeDependency(m_LibraryPath);
            SetupForRuntimeDependency(Path.ChangeExtension(m_LibraryPath, ".pdb"));
            PublicDelayLoadDLLs.Add(m_LibName);
            PublicDefinitions.Add("LUA_BUILD_AS_DLL");
            PublicAdditionalLibraries.Add(Path.ChangeExtension(m_LibraryPath, ".lib"));
        }
        else
        {
            PublicAdditionalLibraries.Add(m_LibraryPath);
        }
    }

    private void GenerateLibrary()
    {
        Console.WriteLine("generating {0} library with cmake...", m_LuaDirName);

        var osPlatform = Environment.OSVersion.Platform;
        if (osPlatform == PlatformID.Win32NT)
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "cmd.exe",
                WorkingDirectory = ModuleDirectory,
                RedirectStandardInput = true,
                UseShellExecute = false
            };
            var process = Process.Start(startInfo);
            using (var writer = process.StandardInput)
            {
                writer.WriteLine("mkdir \"{0}/build\" & pushd \"{0}/build\"", m_LuaDirName);
                writer.Write("cmake -G \"{0}\" ../.. ", "Visual Studio 16 2019");
                var args = new Dictionary<string, string>
                {
                    { "LUA_VERSION", m_LuaVersion },
                    { "LUA_COMPILE_AS_CPP", m_CompileAsCpp ? "1" : "0" },
                };
                foreach (var arg in args)
                    writer.Write(" -D{0}={1}", arg.Key, arg.Value);
                writer.WriteLine();
                writer.WriteLine("popd");
                writer.WriteLine("cmake --build {0}/build --config {1}", m_LuaDirName, m_Config);
            }

            process.WaitForExit();
            return;
        }

        if (osPlatform == PlatformID.MacOSX)
        {
            throw new NotImplementedException();
        }

        if (osPlatform == PlatformID.Unix)
        {
            throw new NotImplementedException();
        }

        throw new NotSupportedException();
    }

    private bool ShouldCompileAsCpp()
    {
        var projectDir = Target.ProjectFile.Directory;
        var configFilePath = projectDir + "/Config/DefaultUnLuaEditor.ini";
        var configFileReference = new FileReference(configFilePath);
        var configFile = FileReference.Exists(configFileReference)
            ? new ConfigFile(configFileReference)
            : new ConfigFile();
        var config = new ConfigHierarchy(new[] { configFile });
        const string section = "/Script/UnLuaEditor.UnLuaEditorSettings";
        bool flag;
        if (config.GetBool(section, "bLuaCompileAsCpp", out flag))
            return flag;
        return true;
    }

    private bool ShouldCompileAsDynamicLib()
    {
        return Target.Platform == UnrealTargetPlatform.Win64
               || Target.Platform == UnrealTargetPlatform.Mac
               || Target.Platform == UnrealTargetPlatform.Android
               || Target.Platform == UnrealTargetPlatform.Linux;
    }

    private void EnsureDirectoryExists(string fileName)
    {
        var dirName = Path.GetDirectoryName(fileName);
        if (!Directory.Exists(dirName))
            Directory.CreateDirectory(dirName);
    }

    private string GetLibraryName()
    {
        if (Target.Platform == UnrealTargetPlatform.Win64)
            return "Lua.dll";
        if (Target.Platform == UnrealTargetPlatform.Mac)
            return "Lua.dylib";
        if (Target.Platform == UnrealTargetPlatform.IOS)
            return "Lua.a";
        return "Lua.so";
    }

    private string GetConfigName()
    {
        if (Target.Configuration == UnrealTargetConfiguration.Debug
            || Target.Configuration == UnrealTargetConfiguration.DebugGame)
            return "Debug";
        return "Release";
    }

    private void SetupForRuntimeDependency(string fullPath)
    {
        if (!File.Exists(fullPath))
            return;
        var fileName = Path.GetFileName(fullPath);
        RuntimeDependencies.Add("$(BinaryOutputDir)/" + fileName, fullPath);
    }
    
    private readonly string m_LuaVersion;
    private readonly string m_Config;
    private readonly string m_LibName;
    private readonly string m_LibDirName;
    private readonly string m_LuaDirName;
    private readonly string m_LibraryPath;
    private readonly bool m_CompileAsCpp;
    private readonly bool m_CompileAsDynamicLib;
}