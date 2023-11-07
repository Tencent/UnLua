# Change Log
All notable changes to this project will be documented in this file.
 
The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [2.3.6] - 2023-11-6
### Added
- 对UE5.3的支持 [#642](https://github.com/Tencent/UnLua/pull/642)

### Fixed
- lua在GC时偶现崩溃的问题 [#626](https://github.com/Tencent/UnLua/issue/626)
- 覆写的函数的Out参数是C++结构体时，Lua不返回会导致崩溃 [#634](https://github.com/Tencent/UnLua/issue/634)
- 日志参数不匹配的问题 [#642](https://github.com/Tencent/UnLua/issue/642)
- 访问来自非native父类的property时检查有效性 [#661](https://github.com/Tencent/UnLua/pull/661)
- 同一个Lua函数绑定多个不同签名的代理导致崩溃 [#660](https://github.com/Tencent/UnLua/pull/660)
- 应该支持从L构造FLuaValue类型 [#666](https://github.com/Tencent/UnLua/issue/666)
- 特定情况下在Lua中调用TArray的Add接口时内存对齐引起的问题 [#668](https://github.com/Tencent/UnLua/issue/668)
- 兼容蓝图Recompile导致FuncMap被清空的情况 [#669](https://github.com/Tencent/UnLua/issue/669)
- 清理一些UE5下的编译警告

### Changed
- FObjectRegistry::Push增加Valid检查 规避一些容器内野指针的情况 [#663](https://github.com/Tencent/UnLua/pull/663)
- 每个FLuaEnv持有独立的ClassRegistry和EnumRegistry

## [2.3.5] - 2023-5-29
### Added
- 对UE5.2的支持
- 增加配置选项[自定义Lua版本](./Docs/CN/Settings.md#自定义Lua版本)
- 增加配置选项[启用FText支持](./Docs/CN/Settings.md#启用FText支持)
- 适配UE5.1的UHT [#600](https://github.com/Tencent/UnLua/issue/600) [#607](https://github.com/Tencent/UnLua/pull/607)
- 禁止在Lua的`Initialize`函数中访问当前`UObject`上的`UFunction`
- 在Lua中`loadstring`失败时的全路径错误信息输出

### Fixed
- ENABLE_PERSISTENT_PARAM_BUFFER模式下 Buffer被递归调用的覆盖 [#563](https://github.com/Tencent/UnLua/issue/563)
- 在启用`AsyncLoadingThread`时，异步加载后触发绑定对象到Lua可能引起崩溃

### Changed
- 调整配置文件名称为`UnLuaSettings.ini` [#596](https://github.com/Tencent/UnLua/pull/596)
- 在Lua中`NewObject`时，Outer传nil时使用`TransientPackage` [#604](https://github.com/Tencent/UnLua/pull/604)
- 当不启用类型检查时，若函数要求有返回值但Lua里不返回，则用默认值作为返回值

### Removed
- 移除 `SUPPORTS_RPC_CALL` 宏

## [2.3.3] - 2023-2-2
### Added
- 增加对`EnhancedInput`输入绑定的支持
- 增加 [启用Insights分析支持](./Docs/CN/Settings.md#启用Insights分析支持) 和 [传参方式](./Docs/CN/Settings.md#默认传参)  配置选项
- 热重载失败时输出错误日志 [#581](https://github.com/Tencent/UnLua/pull/581)
- 自动将启用的`UnLuaExtensions`的脚本加入打包设置
- 支持按需重新加载蓝图枚举，也支持用UnLua.Ref来保持引用 [#585](https://github.com/Tencent/UnLua/issues/585)

### Fixed
- 编辑器使用`Development`环境启动后，加载Lua脚本报错时代码优化导致longjmp崩溃的问题
- 在Lua中遍历TMap字段的Values接口返回值时引起的崩溃 [#583](https://github.com/Tencent/UnLua/issues/583)
- 协程里require脚本加载异常 [#551](https://github.com/Tencent/UnLua/issues/551)
- 加载`socket.http`模块时报错

### Changed
- 在启用类型检查时，非法参数不写入实际参数内存块，避免报错后又在使用时崩溃
- 将`UnLuaExtensions`的启动时间提前，避免在入口脚本里直接访问的时候这些模块还没启动

## [2.3.2] - 2022-12-9

### Added
- 增加配置选项[崩溃时输出Lua堆栈到日志](./Docs/CN/Settings.md#崩溃时输出lua堆栈到日志)
- 针对自`2.2.0`版本以来调整的垃圾回收机制的[说明文档](./Docs/CN/UnLua_Programming_Guide.md#五垃圾回收)

### Fixed
- UObject绑定后的元表和其他表相等判断时结果错误 [#281](https://github.com/Tencent/UnLua/issues/281) [#567](https://github.com/Tencent/UnLua/issues/567)
- 访问UStruct内部的委托会check [#561](https://github.com/Tencent/UnLua/issues/561)
- 多次传递委托类型的参数到同一函数时，可能因为Owner失效而无法回调 [#566](https://github.com/Tencent/UnLua/issues/566)
- UE5.1之后构造`FProperty`报deprecated [#569](https://github.com/Tencent/UnLua/issues/569)
- pairs在参数异常时返回空迭代器，避免lua调试时访问报错
- 热重载upvalue没有生效
- `UObject.Load`传入中文路径时乱码

### Changed
- 在[热重载模式](./Docs/CN/Settings.md#热重载模式)为禁用时，不再加载`HotReload.lua`，不会替换全局的`require`
- 在使用`LoadObject`加载不到对象时不再输出加载失败的日志，而是直接返回`nil`

## [2.3.1] - 2022-11-11

### Added
- 支持UE5.1
- 支持PS5
- 增加 `UnLua::PrintCallStack(L)` 的接口来方便在IDE里断点直接执行输出lua堆栈
- 更多容器和结构体相关的访问保护，增加[悬垂指针检查](./Docs/CN/Settings.md#启用类型检查)选项
- `UnLuaExtensions` 新增可选集成 [lua-protobuf](https://github.com/starwing/lua-protobuf) 和 [lua-rapidjson](https://github.com/xpol/lua-rapidjson)
- 增加 `FLuaEnv` 的 `OnDestroyed` 事件

### Fixed
- Lua报错输出脚本路径如果太长会被截断
- xxx:IsA(UE.UClass) 会报错
- Lua覆写Out返回值时无法返回nil [#539](https://github.com/Tencent/UnLua/issues/539)
- 安装 `Apple ProRes Media` 插件后会导致UnLua启动崩溃 [#534](https://github.com/Tencent/UnLua/issues/534)
- Actor的Struct成员变量在Lua里引用，释放后仍旧可以访问 [#517](https://github.com/Tencent/UnLua/issues/517)
- 在 `print` 时参数过多可能会导致Lua栈溢出的问题 [#543](https://github.com/Tencent/UnLua/pull/543)
- LuaGC使用了未初始化的参数 [#548](https://github.com/Tencent/UnLua/pull/548)
- NullPointer error in function 'CheckPropertyType' [#549](https://github.com/Tencent/UnLua/pull/549)
- 找不到 `UnLua.Input` 模块时不会再check了
- 访问非TArray的结构体数组报错 [#554](https://github.com/Tencent/UnLua/issues/554)
- 服务端 `Possess` 后，新角色上的 `InputComponent` 输入绑定无效 [#553](https://github.com/Tencent/UnLua/issues/553)
- mac打包找不到libLua.dylib问题 [#557](https://github.com/Tencent/UnLua/pull/557)

### Changed
- 在[启用类型检查](./Docs/CN/Settings.md#启用类型检查)时，需要依次返回返回值和Out参数，而不能像旧版本一样忽略不返回

## [2.3.0] - 2022-10-8

### Added
- 支持使用 `UnLua.PackagePath` 的方式来搜索Lua文件，也支持从插件Content目录加载
- 支持Android下的x86_64
- 支持自定义预绑定类型，参考[预绑定类型列表](./Docs/CN/Settings.md#预绑定类型列表)配置选项
- 支持UE5下的蓝图UMG输入绑定，使用新增的 `UnLua.Input` 模块，可以做到更细节的输入绑定
- `UnLua.Ref` 和 `UnLua.Unref` 接口，提供将 `UObject` 生命周期和Lua侧同步的管理机制
- 提升Lua访问UE函数和属性的性能
- [自定义生成Lua模版](./Docs/CN/CustomTemplate.md)

### Fixed
- Mac下编辑器的dylib无法加载
- PushMetatable时会使用旧的metatable [#515](https://github.com/Tencent/UnLua/pull/515)
- Delegate的闭包函数的upvalue无法被gc [#516](https://github.com/Tencent/UnLua/issues/516)
- 在Lua中访问TArray不存在的字段会报stackoverflow
- 自动保存的打包设置没有生效
- UE5下打包后UnLua配置没有正确加载

### Changed
- 默认关闭运行时对`UTF-8 BOM`文件头的加载支持，需要兼容请开启[兼容UTF-8 BOM文件头](./Docs/CN/Settings.md#兼容UTF-8%20BOM文件头)选项

### Removed
- 移除 `AddPackagePath` 接口

## [2.2.4] - 2022-9-1

### Added
- 增加最佳实践工程示例 [Lyra with UnLua](https://github.com/xuyanghuang-tencent/LyraWithUnLua)
- 支持配置按C/C++编译Lua环境
- 支持Lua启动入口脚本配置
- 支持Lua环境手动启动参数
- 默认自动将 `Content/Script` 目录加入打包设置
- 增加一些指针对象的合法性检查
- `UnLua.HotReload` 支持手动指定热重载模块列表
- 支持Commandlet导出蓝图智能提示信息 [#507](https://github.com/Tencent/UnLua/pull/507)

### Fixed
- UE5下的Script编译警告
- 智能提示文件重复生成 [#498](https://github.com/Tencent/UnLua/issues/498)
- 智能提示蓝图类型使用 `_C` 后缀 [#493](https://github.com/Tencent/UnLua/pull/493)
- PIE运行状态下保存对象，可能引起编辑器崩溃 [#489](https://github.com/Tencent/UnLua/pull/489)
- `bAutoStartup` 配置选项没有生效
- 当 `UnLuaHotReload.lua` 不存在时会报错
- 通过C++类绑定的时候使用自动创建脚本功能会崩溃 [#490](https://github.com/Tencent/UnLua/issues/490)
- 修复一些智能提示被过滤了的情况
- 监听嵌套界面里的组件的事件会导致组件无法被回收
- 覆写C++类型的函数后在蓝图编辑器里调用需要刷新节点才能编译过 [#500](https://github.com/Tencent/UnLua/issues/500)
- Lua持有结构体下的TArray字段，在结构体本身被GC后访问该数组会导致崩溃 [#505](https://github.com/Tencent/UnLua/issues/505)
- `TCHAR_TO_XXX` 等宏应该只在行内传参使用 [#508](https://github.com/Tencent/UnLua/pull/508)
- 退出游戏时候可能访问已经被释放的 `UUnLuaManager` 引起的崩溃 [#504](https://github.com/Tencent/UnLua/issues/504)
- UE5下在编辑器运行游戏的同时编译并保存动画蓝图会Crash [#510](https://github.com/Tencent/UnLua/issues/510)

### Changed
- Lua模版文件中使用 `@type` 注解 [#498](https://github.com/Tencent/UnLua/issues/498)
- 使用智能指针保存 `UEnum` 类型指针来区分有效性 [#488](https://github.com/Tencent/UnLua/pull/488)
- Lua源码作为外部第三方模块引入，默认使用C编译
- Lua生成模版中统一使用 `UnLua.Class`，并增加类型注解
- 调整所有LuaLib的异常抛出形式为 `luaL_error` 而不是仅输出错误日志
- 切换场景时不再强制进行LuaGC

## [2.2.3] - 2022-7-15

### Added
- 支持蓝图的`BlueprintFunctionLibrary`绑定到Lua与覆写
- 支持CDO绑定
- 支持自定义[Lua模块定位](./Docs/CN/Settings.md#Lua模块定位器)设置
- 生成Lua模版文件时增加`@class`注解
- UnLua内置API的智能提示
- 生成智能提示时显示更详细的进度条

### Fixed
- 打包DS服务端后，预先放在地图里的绑定过Lua的Actor会导致崩溃 [#479](https://github.com/Tencent/UnLua/issues/479)
- 退出PIE时一些被覆写的Lua函数不会被执行 [#472](https://github.com/Tencent/UnLua/issues/472)
- 切换场景时访问已释放对象上的属性时会引起崩溃 [#482](https://github.com/Tencent/UnLua/issues/482)
- 同一个委托对象传递给不同函数来绑定和解绑，会出现无法解绑的问题 [#471](https://github.com/Tencent/UnLua/issues/471)
- Lua传递给蓝图的FName属性中文会乱码 [#474](https://github.com/Tencent/UnLua/pull/474)
- 传递给Lua模块的`...`参数没有生效
- 退出PIE时父类被覆写的UFunction没有还原
- UE4命名空间的兼容开关没生效
- Editor下PIE判断不准确 [#468](https://github.com/Tencent/UnLua/pull/468)
- 真机上UnLuaExtensions模块启动比UnLuaModule晚，导致luasocket没有加载 [#484](https://github.com/Tencent/UnLua/issues/484)
- 调用静态导出函数的`TCHAR*`类型参数传递为空 [#486](https://github.com/Tencent/UnLua/pull/486)

### Changed
- 移除`UnLua.lua`，`UnLua`可作为全局对象访问，不需要`require "UnLua"`了

## [2.2.2] - 2022-6-17

### Added
- 优化绑定检测流程，避免在大量对象创建时导致性能降低 [#457](https://github.com/Tencent/UnLua/pull/457)
- 通过菜单快速在文件管理器中打开绑定的Lua文件 [#437](https://github.com/Tencent/UnLua/pull/437)
- 防止Lua代码无限循环超时设置 [#428](https://github.com/Tencent/UnLua/pull/428)
- 支持添加多播代理绑定相同脚本不同对象实例函数 [#439](https://github.com/Tencent/UnLua/pull/439)
- 更准确的内存分配统计
- `lua.gc` 控制台命令
- UnLua 运行时/编辑器设置的子菜单
- 编辑器设置支持中文显示

### Fixed
- 命令行 `-server` 启动时 `UnLuaModule` 没有启动 [#440](https://github.com/Tencent/UnLua/issues/440)
- `TArray` 和 `TMap` 进行 `pairs` 遍历时使用引用而不是复制 [#442](https://github.com/Tencent/UnLua/pull/442)
- 实现了FTickableGameObject的对象在Tick里调用自身被Lua覆写的方法会崩溃 [#446](https://github.com/Tencent/UnLua/issues/440)
- 返回 `TSubclassOf<>` 到C++为空 [#445](https://github.com/Tencent/UnLua/issues/445)
- UE4.27下无法通过UE.XXX访问游戏项目模块中导出的原生类型 [#448](https://github.com/Tencent/UnLua/issues/448)
- 从Lua按传递引用到蓝图的TArray引用变成了空Array [#453](https://github.com/Tencent/UnLua/issues/453)
- PIE过程中如果保存了覆写的蓝图，会导致蓝图资源损坏 [#465](https://github.com/Tencent/UnLua/issues/465)
- CDO绑定时需要过滤掉 `SKEL` 类型的对象 [#460](https://github.com/Tencent/UnLua/pull/460)
- 分配在栈上的本地变量会引起 `CacheScriptContainer` 缓存错误导致崩溃 [#455](https://github.com/Tencent/UnLua/issues/455)
- 热重载时报 `invalid TArray/TMap` 的错误
- PIE过程中如果保存了覆写的蓝图，会导致蓝图资源损坏 [#465](https://github.com/Tencent/UnLua/issues/465)
- Linux下带Editor编译报错 [#467](https://github.com/Tencent/UnLua/pull/467)

## [2.2.1] - 2022-5-25

### Added
- TArray/TMap 支持 `pairs` 遍历
- TArray 支持使用 `[]` 访问与获取元素，等同于 `Get()` 和 `Set()`
- TArray/TMap/TSet 的 `Num` 接口，作为 `Length` 的别名
- 容器支持使用 `UStruct` 作为元素
- 增加 `UNLUA_LEGACY_RETURN_ORDER` 配置项，以兼容老项目返回值顺序的问题
- 增加 `UNLUA_LEGACY_BLUEPRINT_PATH` 配置项，以兼容老项目资源路径的问题
- 按住`Alt`点击绑定可以直接快速按蓝图路径填充Lua模块路径到`GetModuleName`

### Fixed
- 从Lua侧返回一个数组给蓝图，可能导致卡死
- UE5下编译报错找不到Tools.DotNETCommon
- TPSProject在以客户端模式运行时报找不到GameMode的问题
- 以-server参数启动时会出现断言 [#415](https://github.com/Tencent/UnLua/pull/415)
- 退出编辑器时候产生崩溃 [#421](https://github.com/Tencent/UnLua/issues/421)
- UE5主菜单不显示菜单按钮的问题 [#422](https://github.com/Tencent/UnLua/issues/422)
- UE5下非const引用参数返回顺序异常
- UE4.25下一些编译报错
- 导出非UENUM的枚举成员类型异常 [#425](https://github.com/Tencent/UnLua/issues/425)
- 使用CustomLoader导致打印堆栈时无法显示文件名 [#429](https://github.com/Tencent/UnLua/pull/429)
- UE5动画通知调用组件Lua覆写函数崩溃 [#430](https://github.com/Tencent/UnLua/issues/430)
- Lua调用的函数返回蓝图结构体会check [#432](https://github.com/Tencent/UnLua/issues/432)
- 执行带返回值的委托时，无法从lua返回值

## [2.2.0] - 2022-5-7

### Added
- 官方交流QQ群：936285107
- 编辑器主界面工具栏的菜单项入口
- 内置一些基础的[控制台命令](Docs/CN/ConsoleCommand.md)
- 配置支持，通过引擎菜单 `项目设置 -> 插件 -> UnLua` 修改运行时和编辑器环境的配置
- 编辑器界面多语言支持，现在可以看到中文菜单了
- 多虚拟机环境支持
- 同名蓝图加载
- `FSoftObjectPtr` 的静态导出缺少的接口 [#392](https://github.com/Tencent/UnLua/issues/392) [#397](https://github.com/Tencent/UnLua/issues/397)
- Standalone模式支持 [#396](https://github.com/Tencent/UnLua/issues/396)
- `GetModuleName` 的路径为空时无法生成模版文件的提示 [#341](https://github.com/Tencent/UnLua/issues/341)
- 增加Enum的 `GetDisplayNameTextByValue` / `GetNameStringByValue` 接口，以支持多语言环境
- 关于窗口和新版本检测
- 演示工程调整为可以以 `Server` / `Client` 方式启动
- 演示工程增加 `TPSGameInstance` 以演示GameInstance的Lua绑定

### Changed
- 原来需要在 `UnLua.Build.cs` 里手动修改的宏定义都可以通过项目设置来配置了
- 默认启用热重载
- `UnLuaEditor` 模块负责生成智能提示信息导出
- `UnLuaTestSuite` 模块提取为独立的插件，省去拷贝插件到自己的工程时还需要手动删除
- 调整Lua源码为 `.cpp` ，以使用C++环境来编译Lua模块
- 调整 `UFunction` 的覆写机制，不再使用`EX_CallLua`字节码
- 禁止从 `UE` 命名空间直接访问蓝图类型，需要使用 `UE.UClass.Load` 来加载
- `UnLua::CreateState` 已经不符合语义，标记为 `DEPRECATED`

### Fixed
- 绑定状态图标没有刷新的问题
- C++的类析构会在luaL_error后被跳过 [#386](https://github.com/Tencent/UnLua/issues/386)
- 开启多窗口，一个有UI展示，一个没有UI展示 [#387](https://github.com/Tencent/UnLua/issues/387)
- 源码版引擎 Test 版本 UStruct::SerializeExpr Crash [#374](https://github.com/Tencent/UnLua/issues/374)
- 某个UObject如果不再被Lua引用，它在Lua添加的委托函数就会失效[#394](https://github.com/Tencent/UnLua/issues/394)
- 接口列表从lua传到c++/蓝图层会丢失一半信息[#398](https://github.com/Tencent/UnLua/issues/398)
- 不同实例的多播委托绑定相同实例函数，清理时崩溃 [#400](https://github.com/Tencent/UnLua/issues/400)
- LUA覆写导致数组传参错误 [#401](https://github.com/Tencent/UnLua/issues/401)
- LUA以Out形式传参给蓝图数组始终为空 [#404](https://github.com/Tencent/UnLua/issues/404)

### Removed
- 移除诸如 `GLuaCxt` / `GReflectionRegistry` / `GObjectReferencer` 的全局变量
- 移除宏定义 `SUPPORT_COMMANDLET` 和 `ENABLE_AUTO_CLEAN_NNATCLASS`
- 移除 `UnLuaFrameWork` 模块
- 移除 `IntelliSenseBP` 模块
- 移除 `IntelliSense` 模块

### PS

我们把内部使用的工具提取成了独立的组件，欢迎点击[链接](https://marketplace.visualstudio.com/items?itemName=operali.lua-booster)体验，也可以通过VSCode应用商店中直接搜索`Lua Booster`安装。

## [2.1.4] - 2022-4-8

### Fixed

- UMG中使用DynamicEntryBox，如果在生成的Entry中执行异步加载，会导致找不到绑定的Object [#379](https://github.com/Tencent/UnLua/issues/379)
- Streaming Level 中的单位有时调用 Lua 实现时，self 是 userdata [#380](https://github.com/Tencent/UnLua/issues/380)
- 开启多窗口，一个有UI展示，一个没有UI展示 [#387](https://github.com/Tencent/UnLua/issues/387)
- 编辑器下PIE结束时可能Crash的问题 [#388](https://github.com/Tencent/UnLua/issues/388)
- UE4.25之前的版本在cook时提示IntProperty initialized not properly

## [2.1.3] - 2022-3-9

### Added

- 支持 UE5.0 Preview 1
- 支持 Blueprint Clustering [#355](https://github.com/Tencent/UnLua/issues/355)
- 在通过UProperty赋值时对Owner的检测，避免写坏到错误的Owner [#344](https://github.com/Tencent/UnLua/issues/344)
- UGameInstanceSubsystem绑定支持 [#345](https://github.com/Tencent/UnLua/pull/345)
- 内置一些常见的导出

### Fixed

- ENABLE_TYPE_CHECK开启情况下，应该只做类型检查，类型兼容的话支持Value设置，跟之前的版本保持一致 [#353](https://github.com/Tencent/UnLua/pull/353)
- UE5地图中放置绑定Lua覆写函数的Actor,在打包后客户端加载时会奔溃 [#325](https://github.com/Tencent/UnLua/issues/325)
- LevelStreaming导致延迟绑定的Actor无法触发ReceiveBeginPlay [#343](https://github.com/Tencent/UnLua/issues/343)
- 引用参数和函数返回值的返回顺序错误 [#361](https://github.com/Tencent/UnLua/issues/361)
- 类型元表被LuaGC时，可能意外导致正在使用的FClassDesc被错误delete [#367](https://github.com/Tencent/UnLua/issues/367)
- 关闭SUPPORTS_RPC_CALL后，参数需要拷贝到重用的Buffer中，避免无法进行引用传递 [#364](https://github.com/Tencent/UnLua/issues/364)
- 委托参数是TArray&时，在lua中修改无效 [#362](https://github.com/Tencent/UnLua/issues/362)
- Actor调用K2_DestroyActor销毁时，lua部分会在注册表残留 [#372](https://github.com/Tencent/UnLua/issues/372)
- 子类被销毁时，可能会令父类的Lua绑定失效 [#375](https://github.com/Tencent/UnLua/issues/375)
- 4.27在不启用Async Loading Thread Enabled的情况下打包后加载地图会导致Crash的问题 [#365](https://github.com/Tencent/UnLua/issues/365)

## [2.1.2] - 2022-1-17

### Added

- 更多的单元测试和回归测试
- 增加使用FActorSpawnParameters扩展参数的SpawnActorEx [#333](https://github.com/Tencent/UnLua/pull/333)

### Fixed

- 一些默认参数的写法没有分析出来 [#323](https://github.com/Tencent/UnLua/issues/323)
- 从Lua模板创建中文脚本时编码错误的问题 [#322](https://github.com/Tencent/UnLua/issues/322)
- C++调用Lua覆写带返回值的函数会崩溃 [#328](https://github.com/Tencent/UnLua/issues/328)
- PIE下GameInstance的Initialize会被调用两次 [#326](https://github.com/Tencent/UnLua/issues/326)
- 支持UENUM带表达式的项的默认值 [#331](https://github.com/Tencent/UnLua/issues/331)
- Hotfix模块失效的问题 [#334](https://github.com/Tencent/UnLua/pull/334)

## [2.1.1] - 2021-12-29

### Added

- 更多的单元测试和回归测试
- UDataTableFunctionLibrary.GetDataTableRowFromName

### Fixed

- 生成Lua模版文件时需要按 `GetModuleName` 指定的路径生成 [#301](https://github.com/Tencent/UnLua/issues/301)
- 在蓝图中调用被Lua覆写的蓝图函数，无法拿到正确的返回值 [#300](https://github.com/Tencent/UnLua/issues/300)
- 在 `Monolithic` 下Lua不应定义宏 `LUA_BUILD_AS_DLL` [#308](https://github.com/Tencent/UnLua/pull/308)
- 真机环境下被覆写的UFunction被GC后，指针重用会导致传参错误或崩溃 [#299](https://github.com/Tencent/UnLua/issues/299)
- 打开多个蓝图编辑窗口时，菜单栏操作对象混乱 [#310](https://github.com/Tencent/UnLua/issues/310)
- 部分情况下为TArray创建拷贝至lua时会产生崩溃 [#303](https://github.com/Tencent/UnLua/issues/303)
- 访问过父类不存在的字段，后续在子类中也只能取到nil [#314](https://github.com/Tencent/UnLua/pull/314)

### Removed

- 移除宏定义 `SUPPORT_COMMANDLET`

## [2.1.0] - 2021-12-6

### Added

- 支持最新的UE5抢先体验版
- 基于UE的自动化测试系统的全API测试、回归测试覆盖
- 支持自定义加载器，允许用户自己扩展实现Lua文件的查找与加载逻辑
- 自定义加载器的示例教程
- 编辑器下增加了Toolbar用来进行快速绑定/解绑Lua，以及一些常用功能入口
- 增加 `UUnLuaLatentAction` 用于包装异步行为，支持 `SetTickableWhenPaused` 
- 支持使用 `ADD_STATIC_PROPERTY` 宏来导出静态成员变量
- 支持在Lua中直接访问 `EKeys`
- 编辑器下动态绑定一个静态绑定的对象，会有警告日志避免误操作
- `UnLuaDefaultParamCollector` 模块的默认值生成现在支持 `AutoCreateRefTerm` 标记

### Changed

- 在Lua中访问UE对象统一使用`UE`，考虑到向后兼容，原来的`UE4`继续保留
- 统一结构类型的构造参数表达，与UE相同
  - FVector/FIntVector/FVector2D/FVector4/FIntPoint
  - FColor/FLinearColor
  - FQuat/FRotator/FTransform
- 统一UObject在Lua侧IsValid的语义，与UE相同

### Fixed
- Lua中调用UClass的方法提示方法为nil [#274](https://github.com/Tencent/UnLua/issues/274)
- 游戏世界暂停后，在协程里的Delay后不继续执行 [#276](https://github.com/Tencent/UnLua/pull/276)
- 在非主线程崩溃时不应该访问Lua堆栈 [#278](https://github.com/Tencent/UnLua/pull/278)
- 无法将FVector/FRotator/FTransform等类型的函数参数保存到lua对象中 [#279](https://github.com/Tencent/UnLua/issues/279)
- 调用Lua时参数传递顺序异常 [#280](https://github.com/Tencent/UnLua/issues/280)
- require不存在的lib会崩 [#284](https://github.com/Tencent/UnLua/issues/284)
- 蓝图TMap的FindRef错误 [#286](https://github.com/Tencent/UnLua/issues/286)
- UMG里Image用到的Texture内存泄漏 [#288](https://github.com/Tencent/UnLua/issues/288)
- UClass在被UEGC后没有释放相应的绑定 [#289](https://github.com/Tencent/UnLua/issues/289)
- 调用K2_DestroyActor后，立刻调用IsValid会返回true [#292](https://github.com/Tencent/UnLua/issues/292)
- 关闭RPC会导致部分函数在非Editor模式下crash [#293](https://github.com/Tencent/UnLua/issues/293)
- 索引DataTable时，如果没有这个key，会得到异常的返回值 [#298](https://github.com/Tencent/UnLua/pull/298)


### Removed

- 移除宏定义 `CHECK_BLUEPRINTEVENT_FOR_NATIVIZED_CLASS`
- 移除宏定义 `CLEAR_INTERNAL_NATIVE_FLAG_DURING_DUPLICATION`

## [2.0.1] - 2021-10-20

### Added

1. UMG释放引用教程
2. Lua热重载机制的开关
3. 部分英文文档

### Fixed

1. 首次切换场景到新场景的Actor的ReceiveBeginPlay没有调用到的问题 #268
2. 不同Actor绑定到同一个Lua脚本没有生效的问题 #269
3. 在编辑器中绑定某些对象时会报class is out of data的问题
4. 在Lua中获取对象属性异常时可能会导致Crash的问题
5. 在编辑器通过-game运行游戏时RestartLevel会导致Crash的问题

## [2.0.0] - 2021-09-18

1. 调整了UObject Ref 的释放机制
2. 更新并适配 Lua 5.4.2
3. 支持对非必要过滤对象的绑定过滤（包括load阶段产生的临时对象、抽象对象等）
4. 增加中文说明文档
5. 增加 Hotfix 功能（增加UnLuaFramework Module，并且使用时需要添加Content/Script 目录下的 G6HotfixHelper.lua/G6Hotfix.lua）
6. 修复若干bugs（参考commits）

## [1.2.0] - 2021-02-09

fix some bugs, could see details from commits

## [1.1.0] - 2020-04-04

Fix the crash bug caused incorrect userdata address.

## [1.0.0] - 2019-08-13

First version of UnLua.