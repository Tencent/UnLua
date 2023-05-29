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
using System.Linq;
using System.Reflection;
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

        m_LuaVersion = GetLuaVersion();
        m_Config = GetConfigName();
        m_LibName = GetLibraryName();
        m_BuildSystem = GetBuildSystem();
        m_CompileAsCpp = ShouldCompileAsCpp();
        m_LibDirName = string.Format("lib-{0}", m_CompileAsCpp ? "cpp" : "c");
        m_LuaDirName = m_LuaVersion;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, m_LuaDirName, "src"));

        var buildMethodName = "BuildFor" + Target.Platform;

        var buildMethod = GetType().GetMethod(buildMethodName, BindingFlags.Instance | BindingFlags.NonPublic);
        if (buildMethod == null)
            throw new NotSupportedException(buildMethodName);

        buildMethod.Invoke(this, Array.Empty<object>());
    }

    #region Build for Platforms

    private void BuildForWin64()
    {
        var dllPath = GetLibraryPath();
        var libPath = Path.ChangeExtension(dllPath, ".lib");
        var pdbPath = Path.ChangeExtension(dllPath, ".pdb");
        var dirPath = Path.GetDirectoryName(dllPath);

        var isDebug = m_Config == "Debug";
        var dstFiles = new List<string> { dllPath, libPath };
        if (isDebug)
            dstFiles.Add(pdbPath);

        if (!dstFiles.All(File.Exists))
        {
            var buildDir = CMake();
            CopyDirectory(Path.Combine(buildDir, m_Config), dirPath);
        }

        PublicDefinitions.Add("LUA_BUILD_AS_DLL");
        PublicDelayLoadDLLs.Add(m_LibName);
        PublicAdditionalLibraries.Add(libPath);

        SetupForRuntimeDependency(dllPath, "Win64");
        if (isDebug)
            SetupForRuntimeDependency(pdbPath, "Win64");
    }

    private void BuildForAndroid()
    {
        var ndkRoot = Environment.GetEnvironmentVariable("NDKROOT");
        if (ndkRoot == null)
            throw new BuildException("can't find NDKROOT");

        var toolchain = GetAndroidToolChain();
        var ndkApiLevel = toolchain.GetNdkApiLevelInt(21);
        Console.WriteLine("toolchain.GetNdkApiLevelInt=", ndkApiLevel);
        var abiNames = new[] { "armeabi-v7a", "arm64-v8a", "x86_64" };
        foreach (var abiName in abiNames)
        {
            var libFile = GetLibraryPath(abiName);
            PublicAdditionalLibraries.Add(libFile);
            if (File.Exists(libFile))
                continue;

            EnsureDirectoryExists(libFile);
            var args = new Dictionary<string, string>
            {
                { "CMAKE_TOOLCHAIN_FILE", Path.Combine(ndkRoot, "build/cmake/android.toolchain.cmake") },
                { "ANDROID_ABI", abiName },
                { "ANDROID_PLATFORM", "android-" + ndkApiLevel }
            };
            var buildDir = CMake(args);
            var buildFile = Path.Combine(buildDir, m_LibName);
            File.Copy(buildFile, libFile, true);
        }
    }

    private IAndroidToolChain GetAndroidToolChain()
    {
#if UE_5_2_OR_LATER
        var ueBuildPlatformType = Assembly.GetAssembly(typeof(IAndroidToolChain)).GetType("UnrealBuildTool.UEBuildPlatform");
        var getBuildPlatformMethod = ueBuildPlatformType.GetMethod("GetBuildPlatform", BindingFlags.Static | BindingFlags.Public);
        var androidBuildPlatform = getBuildPlatformMethod.Invoke(null, new object[] { UnrealTargetPlatform.Android });
        var createTempToolChainForProjectMethod = androidBuildPlatform.GetType().GetMethod("CreateTempToolChainForProject");
        var toolchain = (IAndroidToolChain)createTempToolChainForProjectMethod.Invoke(androidBuildPlatform, new object[] { Target.ProjectFile });
#else
        var toolchain = AndroidExports.CreateToolChain(Target.ProjectFile);
#endif
        return toolchain;
    }

    private void BuildForLinux()
    {
        var libFile = GetLibraryPath();
        PublicAdditionalLibraries.Add(libFile);
        if (File.Exists(libFile))
            return;

        var env = Environment.GetEnvironmentVariable("LINUX_MULTIARCH_ROOT");
        if (string.IsNullOrEmpty(env))
            throw new BuildException("LINUX_MULTIARCH_ROOT environment variable needed.");

        EnsureDirectoryExists(libFile);
        var sysName = "x86_64-unknown-linux-gnu";
        var sysRoot = Path.Combine(env, sysName);
        var clangPath = Path.Combine(env, sysName, @"bin\clang.exe");
        var clangPlusPlusPath = Path.Combine(env, sysName, @"bin\clang++.exe");
        var args = new Dictionary<string, string>
        {
            { "CMAKE_SYSROOT", sysRoot },
            { "CMAKE_SYSTEM_NAME", "Linux" },
            { "CMAKE_C_COMPILER_TARGET", sysName },
            { "CMAKE_C_COMPILER:PATH", clangPath },
            { "CMAKE_CXX_COMPILER:PATH", clangPlusPlusPath },
            { "CMAKE_CXX_COMPILER_TARGET", sysName },
        };
        var buildDir = CMake(args);
        var buildFile = Path.Combine(buildDir, m_LibName);
        File.Copy(buildFile, libFile, true);
    }

    private void BuildForLinuxAArch64()
    {
        var libFile = GetLibraryPath();
        PublicAdditionalLibraries.Add(libFile);
        if (File.Exists(libFile))
            return;

        var env = Environment.GetEnvironmentVariable("LINUX_MULTIARCH_ROOT");
        if (string.IsNullOrEmpty(env))
            throw new BuildException("LINUX_MULTIARCH_ROOT environment variable needed.");

        EnsureDirectoryExists(libFile);
        var sysName = "aarch64-unknown-linux-gnueabi";
        var sysRoot = Path.Combine(env, sysName);
        var clangPath = Path.Combine(env, sysName, @"bin\clang.exe");
        var clangPlusPlusPath = Path.Combine(env, sysName, @"bin\clang++.exe");
        var args = new Dictionary<string, string>
        {
            { "CMAKE_SYSROOT", sysRoot },
            { "CMAKE_SYSTEM_NAME", "Linux" },
            { "CMAKE_C_COMPILER_TARGET", sysName },
            { "CMAKE_C_COMPILER:PATH", clangPath },
            { "CMAKE_CXX_COMPILER:PATH", clangPlusPlusPath },
            { "CMAKE_CXX_COMPILER_TARGET", sysName },
        };
        var buildDir = CMake(args);
        var buildFile = Path.Combine(buildDir, m_LibName);
        File.Copy(buildFile, libFile, true);
    }

    private void BuildForMac()
    {
#if UE_5_2_OR_LATER
        var abiName = Target.Architecture.ToString();
#else
        var abiName = Target.Architecture;
#endif
        var libFile = GetLibraryPath(abiName);
        if (!File.Exists(libFile))
        {
            EnsureDirectoryExists(libFile);
            var args = new Dictionary<string, string>
            {
                { "CMAKE_OSX_ARCHITECTURES", abiName }
            };
            var buildDir = CMake(args);
            var buildFile = Path.Combine(buildDir, m_LibName);
            File.Copy(buildFile, libFile, true);
        }

        PublicDefinitions.Add("LUA_USE_MACOSX");
        PublicAdditionalLibraries.Add(libFile);
        RuntimeDependencies.Add(libFile);
    }

    private void BuildForIOS()
    {
        var libFile = GetLibraryPath("arm64");
        if (!File.Exists(libFile))
        {
            var args = new Dictionary<string, string>
            {
                { "CMAKE_TOOLCHAIN_FILE", Path.Combine(ModuleDirectory, "toolchain/ios.toolchain.cmake") },
                { "PLATFORM", "OS64" }
            };
            var buildDir = CMake(args);
            var buildFile = Path.Combine(buildDir, m_Config + "-iphoneos", m_LibName);
            EnsureDirectoryExists(libFile);
            File.Copy(buildFile, libFile, true);
        }

        PublicAdditionalLibraries.Add(libFile);
    }

    private void BuildForPS5()
    {
        var libFile = GetLibraryPath();
        PublicAdditionalLibraries.Add(libFile);
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, m_LuaDirName, "src-ps5"));

        if (File.Exists(libFile))
            return;

        var sceRootDir = Environment.GetEnvironmentVariable("SCE_ROOT_DIR");
        if (string.IsNullOrEmpty(sceRootDir))
            throw new BuildException("SCE_ROOT_DIR environment variable needed.");

        var sceSdkDir = Environment.GetEnvironmentVariable("SCE_PROSPERO_SDK_DIR");
        if (string.IsNullOrEmpty(sceSdkDir))
            throw new BuildException("SCE_PROSPERO_SDK_DIR environment variable needed.");

        EnsureDirectoryExists(libFile);
        var sysRoot = Path.Combine(sceSdkDir, @"target");
        var clangPath = Path.Combine(sceSdkDir, @"host_tools/bin/prospero-clang.exe");
        var clangPlusPlusPath = Path.Combine(sceSdkDir, @"host_tools/bin/prospero-clang.exe");
        var arPath = Path.Combine(sceSdkDir, @"host_tools/bin/prospero-llvm-ar.exe");
        var args = new Dictionary<string, string>
        {
            { "CMAKE_TOOLCHAIN_FILE", Path.Combine(sceRootDir, @"Prospero\Tools\CMake\PS5.cmake") },
            { "PS5", "1" }
        };
        var buildDir = CMake(args);
        var buildFile = Path.Combine(buildDir, m_Config, m_LibName);
        File.Copy(buildFile, libFile, true);
    }

    #endregion

    private string CMake(Dictionary<string, string> extraArgs = null)
    {
        if (extraArgs == null)
            extraArgs = new Dictionary<string, string>();

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
                var buildDir = string.Format("\"{0}/build\"", m_LuaDirName);
                writer.WriteLine("rmdir /s /q {0} &mkdir {0} &pushd {0}", buildDir);
                if (string.IsNullOrEmpty(m_BuildSystem))
                    writer.Write("cmake ../.. ");
                else
                    writer.Write("cmake -G \"{0}\" ../.. ", m_BuildSystem);
                var args = new Dictionary<string, string>
                {
                    { "LUA_VERSION", m_LuaVersion },
                    { "LUA_COMPILE_AS_CPP", m_CompileAsCpp ? "1" : "0" },
                };
                foreach (var arg in args)
                    writer.Write(" -D{0}=\"{1}\"", arg.Key, arg.Value);
                foreach (var arg in extraArgs)
                    writer.Write(" -D{0}=\"{1}\"", arg.Key, arg.Value);
                writer.WriteLine();
                writer.WriteLine("popd");
                writer.WriteLine("cmake --build {0}/build --config {1}", m_LuaDirName, m_Config);
            }

            process.WaitForExit();

            return Path.Combine(ModuleDirectory, m_LuaDirName, "build");
        }

        if (osPlatform == PlatformID.Unix || osPlatform == PlatformID.MacOSX)
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "/bin/bash",
                WorkingDirectory = ModuleDirectory,
                RedirectStandardInput = true,
                UseShellExecute = false
            };
            var process = Process.Start(startInfo);
            using (var writer = process.StandardInput)
            {
                var buildDir = string.Format("\"{0}/build\"", m_LuaDirName);
                writer.WriteLine("rm -rf {0} &&mkdir {0} &&cd {0}", buildDir);
                writer.Write("cmake -G \"{0}\" ../.. ", m_BuildSystem);
                var args = new Dictionary<string, string>
                {
                    { "LUA_VERSION", m_LuaVersion },
                    { "LUA_COMPILE_AS_CPP", m_CompileAsCpp ? "1" : "0" },
                };
                foreach (var arg in args)
                    writer.Write(" -D{0}={1}", arg.Key, arg.Value);
                foreach (var arg in extraArgs)
                    writer.Write(" -D{0}={1}", arg.Key, arg.Value);
                writer.WriteLine();
                writer.WriteLine("cd ../..");
                writer.WriteLine("cmake --build {0}/build --config {1}", m_LuaDirName, m_Config);
            }

            process.WaitForExit();

            return Path.Combine(ModuleDirectory, m_LuaDirName, "build");
        }

        throw new NotSupportedException();
    }

    private static void CopyDirectory(string srcDir, string dstDir)
    {
        var dir = new DirectoryInfo(srcDir);
        if (!dir.Exists)
            throw new DirectoryNotFoundException(dir.FullName);

        var dirs = dir.GetDirectories();
        Directory.CreateDirectory(dstDir);

        foreach (var file in dir.GetFiles())
        {
            var targetFilePath = Path.Combine(dstDir, file.Name);
            file.CopyTo(targetFilePath, true);
        }

        foreach (var subDir in dirs)
        {
            var newDstDir = Path.Combine(dstDir, subDir.Name);
            CopyDirectory(subDir.FullName, newDstDir);
        }
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
        return false;
    }

    private string GetLuaVersion()
    {
        var projectDir = Target.ProjectFile.Directory;
        var configFilePath = projectDir + "/Config/DefaultUnLuaEditor.ini";
        var configFileReference = new FileReference(configFilePath);
        var configFile = FileReference.Exists(configFileReference)
            ? new ConfigFile(configFileReference)
            : new ConfigFile();
        var config = new ConfigHierarchy(new[] { configFile });
        const string section = "/Script/UnLuaEditor.UnLuaEditorSettings";
        string version;
        if (config.GetString(section, "LuaVersion", out version))
            return version;
        return "lua-5.4.3";
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
            return "libLua.dylib";
        return "libLua.a";
    }

    private string GetLibraryPath(string architecture = null)
    {
        if (architecture == null)
            architecture = string.Empty;
        return Path.Combine(ModuleDirectory, m_LuaDirName, m_LibDirName, Target.Platform.ToString(), architecture, m_Config, m_LibName);
    }

    private string GetConfigName()
    {
        if (Target.Configuration == UnrealTargetConfiguration.Debug
            || Target.Configuration == UnrealTargetConfiguration.DebugGame)
            return "Debug";
        return "Release";
    }

    private string GetBuildSystem()
    {
        var osPlatform = Environment.OSVersion.Platform;
        if (osPlatform == PlatformID.Win32NT)
        {
            if (Target.Platform.IsInGroup(UnrealPlatformGroup.Linux))
                return "Ninja";
            if (Target.Platform.IsInGroup(UnrealPlatformGroup.Android))
                return "Ninja";
            if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
            {
                if (Target.WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2019)
                    return "Visual Studio 16 2019";
#if UE_4_27_OR_LATER
                if (Target.WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2022)
                    return "Visual Studio 17 2022";
#endif
            }
        }

        if (osPlatform == PlatformID.Unix)
        {
            if (Target.Platform.IsInGroup(UnrealPlatformGroup.IOS))
                return "Xcode";
            return "Unix Makefiles";
        }

        return null;
    }

    private void SetupForRuntimeDependency(string fullPath, string platform)
    {
        if (!File.Exists(fullPath))
            return;
        var fileName = Path.GetFileName(fullPath);
        var dstPath = Path.Combine("$(ProjectDir)", "Binaries", platform, fileName);
        RuntimeDependencies.Add(dstPath, fullPath);
    }

    private readonly string m_LuaVersion;
    private readonly string m_Config;
    private readonly string m_LibName;
    private readonly string m_LibDirName;
    private readonly string m_LuaDirName;
    private readonly string m_BuildSystem;
    private readonly bool m_CompileAsCpp;
}