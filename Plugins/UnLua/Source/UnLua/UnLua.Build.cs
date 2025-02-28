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
#if UE_5_2_OR_LATER
        // 禁用 IWYU（Include What You Use）支持
        IWYUSupport = IWYUSupport.None;
#else
        // 禁用 IWYU（Include What You Use）支持
        bEnforceIWYU = false;
#endif
        // 使用显式或共享的 PCH（预编译头）
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // 仅在发布版本中优化代码
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

        // 公共包含路径
        PublicIncludePaths.AddRange(
            new string[]
            {
            }
        );

        // 私有包含路径
        PrivateIncludePaths.AddRange(
            new[]
            {
                "UnLua/Private",
            }
        );

        // 公共依赖模块
        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "InputCore",
                "Lua"
            }
        );

        // 添加私有目录到公共包含路径
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        // 如果构建的是编辑器
        if (Target.bBuildEditor)
        {
            // 从不优化代码
            OptimizeCode = CodeOptimization.Never;
            // 添加编辑器依赖模块
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        // 获取项目目录和配置文件路径
        var projectDir = Target.ProjectFile.Directory;
        var configFilePath = projectDir + "/Config/DefaultUnLuaEditor.ini";
        var configFileReference = new FileReference(configFilePath);
        var configFile = FileReference.Exists(configFileReference) ? new ConfigFile(configFileReference) : new ConfigFile();
        var config = new ConfigHierarchy(new[] { configFile });
        const string section = "/Script/UnLuaEditor.UnLuaEditorSettings";

        // 定义加载布尔配置的委托
        Action<string, string, bool> loadBoolConfig = (key, macro, defaultValue) =>
        {
            bool flag;
            if (!config.GetBool(section, key, out flag))
                flag = defaultValue;
            PublicDefinitions.Add(string.Format("{0}={1}", macro, (flag ? "1" : "0")));
        };

        // 定义加载字符串配置的委托
        Action<string, string, string> loadStringConfig = (key, macro, defaultValue) =>
        {
            string value;
            if (!config.GetString(section, key, out value))
                value = defaultValue;
            PublicDefinitions.Add(string.Format("{0}={1}", macro, value));
        };

        // 加载各种配置
        loadBoolConfig("bAutoStartup", "AUTO_UNLUA_STARTUP", true);
        loadBoolConfig("bEnableDebug", "UNLUA_ENABLE_DEBUG", false);
        loadBoolConfig("bEnablePersistentParamBuffer", "ENABLE_PERSISTENT_PARAM_BUFFER", true);
        loadBoolConfig("bEnableTypeChecking", "ENABLE_TYPE_CHECK", true);
        loadBoolConfig("bEnableUnrealInsights", "ENABLE_UNREAL_INSIGHTS", false);
        loadBoolConfig("bEnableCallOverriddenFunction", "ENABLE_CALL_OVERRIDDEN_FUNCTION", true);
        loadBoolConfig("bEnableFText", "UNLUA_ENABLE_FTEXT", false);
        loadBoolConfig("bLuaCompileAsCpp", "LUA_COMPILE_AS_CPP", false);
        loadBoolConfig("bWithUE4Namespace", "WITH_UE4_NAMESPACE", true);
        loadBoolConfig("bLegacyReturnOrder", "UNLUA_LEGACY_RETURN_ORDER", false);
        loadBoolConfig("bLegacyBlueprintPath", "UNLUA_LEGACY_BLUEPRINT_PATH", false);
        loadBoolConfig("bLegacyAllowUTF8WithBOM", "UNLUA_LEGACY_ALLOW_BOM", false);
        loadBoolConfig("bLegacyArgsPassing", "UNLUA_LEGACY_ARGS_PASSING", true);
        loadStringConfig("LuaVersion", "UNLUA_LUA_VERSION", "lua-5.4.3");

        // 加载热重载模式配置
        string hotReloadMode;
        if (!config.GetString(section, "HotReloadMode", out hotReloadMode))
            hotReloadMode = "Manual";

        var withHotReload = hotReloadMode != "Never";
        PublicDefinitions.Add("UNLUA_WITH_HOT_RELOAD=" + (withHotReload ? "1" : "0"));

        // 如果启用了 LuaCompat 插件，添加其包含路径
        if (IsPluginEnabled("LuaCompat"))
            PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/Lua/lua-compat-5.3/c-api"));
    }

    // 检查插件是否启用
    private bool IsPluginEnabled(string name)
    {
        var engineDir = DirectoryReference.FromString(EngineDirectory);
        var projectDir = Target.ProjectFile.Directory;
        var projectDesc = ProjectDescriptor.FromFile(Target.ProjectFile);

        foreach (var plugin in Plugins.ReadAvailablePlugins(engineDir, projectDir, null))
        {
            if (plugin.Name != name)
                continue;
            return Plugins.IsPluginEnabledForTarget(plugin, projectDesc, Target.Platform, Target.Configuration, Target.Type);
        }

        return false;
    }
}