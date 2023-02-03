![LOGO](./Docs/Images/UnLua.png)

[![license](https://img.shields.io/badge/license-MIT-blue)](https://github.com/Tencent/UnLua/blob/master/LICENSE.TXT)
[![release](https://img.shields.io/github/v/release/Tencent/UnLua)](https://github.com/Tencent/UnLua/releases)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/Tencent/UnLua/pulls)

# 概述
**UnLua**是适用于UE的一个高度优化的**Lua脚本解决方案**。它遵循UE的编程模式，功能丰富且易于学习，UE程序员可以零学习成本使用。

# 在UE中使用Lua
* 直接访问所有的UCLASS, UPROPERTY, UFUNCTION, USTRUCT, UENUM，无须胶水代码。
* 替换蓝图中定义的实现 ( Event / Function )。
* 处理各类事件通知 ( Replication / Animation / Input )。

更详细的功能介绍请查看[功能清单](Docs/CN/Features.md)。

# 优化特性
* UFUNCTION调用，包括持久化参数缓存、优化的参数传递、优化的非常量引用和返回值处理。
* 访问容器类（TArray, TSet, TMap），内存布局与引擎一致，Lua Table和容器之间不需要转换。
* 高效的结构体创建、访问、GC。
* 支持自定义静态导出类、成员变量、成员函数、全局函数、枚举。

# 平台支持
* 运行平台：Windows / Android / iOS / Linux / OSX
* 引擎版本：Unreal Engine 4.17.x - Unreal Engine 5.x

**注意**: 4.17.x 和 4.18.x 版本需要对 Build.cs 做一些修改。

# 快速开始
## 安装
  1. 复制 `Plugins` 目录到你的UE工程根目录。
  2. 重新启动你的UE工程

## 开始UnLua之旅
**注意**: 如果你是一位UE萌新，推荐使用更详细的[图文版教学](Docs/CN/Quickstart_For_UE_Newbie.md)继续以下步骤。
  1. 新建蓝图后打开，在UnLua工具栏中选择 `绑定`（可同时按住`Alt`键自动生成第2步的路径）
  2. 在接口的 `GetModule` 函数中填入Lua文件路径，如 `GameModes.BP_MyGameMode`
  3. 选择UnLua工具栏中的 `创建Lua模版文件`
  4. 打开 `Content/Script/GameModes/BP_MyGameMode.lua` 编写你的代码

# 更多示例
  * [01_HelloWorld](Content/Script/Tutorials/01_HelloWorld.lua) 快速开始的例子
  * [02_OverrideBlueprintEvents](Content/Script/Tutorials/02_OverrideBlueprintEvents.lua) 覆盖蓝图事件（Overridden Functions）
  * [03_BindInputs](Content/Script/Tutorials/03_BindInputs.lua) 输入事件绑定
  * [04_DynamicBinding](Content/Script/Tutorials/04_DynamicBinding.lua) 动态绑定
  * [05_BindDelegates](Content/Script/Tutorials/05_BindDelegates.lua) 委托的绑定、解绑、触发
  * [06_NativeContainers](Content/Script/Tutorials/06_NativeContainers.lua) 引擎层原生容器访问
  * [07_CallLatentFunction](Content/Script/Tutorials/07_CallLatentFunction.lua) 在协程中调用 `Latent` 函数
  * [08_CppCallLua](Content/Script/Tutorials/08_CppCallLua.lua) 从C++调用Lua
  * [09_StaticExport](Content/Script/Tutorials/09_StaticExport.lua) 静态导出自定义类型到Lua使用
  * [10_Replications](Content/Script/Tutorials/10_Replications.lua) 覆盖网络复制事件
  * [11_ReleaseUMG](Content/Script/Tutorials/11_ReleaseUMG.lua) 释放UMG相关对象
  * [12_CustomLoader](Content/Script/Tutorials/12_CustomLoader.lua) 自定义加载器
  * [13_AnimNotify](Content/Script/Tutorials/AN_FootStep.lua) 动画通知

# 最佳实践示例

[Lyra with UnLua](https://github.com/xuyanghuang-tencent/LyraWithUnLua) 基于UE官方 **Lyra初学者游戏包** 的完整示例项目，目前正在施工中

# 文档

常用文档：[设置选项](Docs/CN/Settings.md) | [调试](Docs/CN/Debugging.md) | [智能提示](Docs/CN/IntelliSense.md) | [控制台命令](Docs/CN/ConsoleCommand.md) | [FAQ](Docs/CN/FAQ.md)

详细介绍：
* [编程指南](Docs/CN/UnLua_Programming_Guide.md)：介绍 UnLua 的主要功能和编程模式
* [插件与模块](Docs/CN/Plugins_And_Modules.md)：介绍 Plugins 目录下的插件列表以及它们所包含的模块
* [功能清单](Docs/CN/Features.md)：更详细的功能列表
* [实现原理](Docs/CN/How_To_Implement_Overriding.md)：介绍 UnLua 的两种覆盖机制
* [API](Docs/CN/API.md)：更详细的 UnLua API 说明

# 技术支持
- 官方交流QQ群：936285107
- 推荐VSCode插件：[Lua Booster](https://marketplace.visualstudio.com/items?itemName=operali.lua-booster)