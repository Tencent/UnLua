# UnLua API

## C++ API

### 全局变量
* `UnLua::GLuaSrcRelativePath` Lua源码文件相对于Content目录的相对路径

* `UnLua::GLuaSrcFullPath` Lua源码文件所在的绝对路径

* `FLuaContext::GLuaCxt` 全局Lua上下文

### 全局函数
命名空间：UnLua
* `Call` 调用某个全局 `function`

* `CallTableFunc` 调用全局 `table` 里的某个 `function`

* `Get` 从Lua堆栈的某个索引位置上获取指定类型的数据

* `Push` / PushXXX 向Lua堆栈中压入一个指定类型的数据

* `IsType` 判断Lua堆栈上指定索引位置上的对象类型

* `CreateState` 从 `GLuaCxt` 创建一个 `lua_State`

* `GetState` 获取 `GLuaCxt` 中的 `lua_State`

* `Startup` 启动UnLua环境

* `Shutdown` 停止UnLua环境

* `LoadFile` 从指定相对路径加载一个Lua文件，但不运行

* `RunFile` 从指定相对路径运行一个Lua文件

* `LoadChunk` 加载一个Lua Chunk，但不运行

* `RunChunk` 运行指定的Lua Chunk

* `ReportLuaCallError` 报告Lua错误，如果注册了自定义错误处理委托则会仅执行委托，否则使用UnLua默认的 `UE_LOG` 输出

* `GetStackVariables` 获取当前堆栈上所有相关的变量、`upvalue` 信息

* `GetLuaCallStack` 获取当前Lua调用栈的字符串

### 全局委托
FUnLuaDelegates
* `OnLuaStateCreated` 当一个Lua虚拟机被创建出来时候触发，可以拿到其对应的 `lua_State`

* `OnLuaContextInitialized` 当 `FLuaContext` 初始化完成后触发

* `OnPreLuaContextCleanup` 当 `FLuaContext` 开始回收清理前触发

* `OnPostLuaContextCleanup` 当 `FLuaContext` 完成回收清理后触发

* `OnPreStaticallyExport` 当 `FLuaContext` 开始向 `lua_State` 注册静态导出的类型之前触发

* `OnObjectBinded` 当某个 `UObject` 和Lua绑定时触发

* `OnObjectUnbinded` 当某个 `UObject` 的Lua绑定被解绑时触发

* `ReportLuaCallError` 支持注册自定义Lua报错处理委托到UnLua，当Lua报错时会调用这个委托

* `ConfigureLuaGC` 支持注册自定义LuaGC配置委托到UnLua，覆盖UnLua默认传递给 `lua_gc` 的参数

* `LoadLuaFile` 支持注册自定义的Lua文件加载器委托到UnLua，实现自定义Lua文件加载机制

### 接口
* `UUnLuaInterface` UnLua的核心接口，用来标记和识别需要绑定到Lua的对象

### 类
* `FLuaContext` 封装了整个UnLua运行环境上下文

* `FLuaIndex` 对Lua堆栈索引的简单封装

* `FLuaTable` 对Lua侧 `table` 数据类型的封装，提供类似Lua中"`[]`"的访问机制，也支持调用指定 `function`

* `FLuaValue` 对Lua各种值类型的封装

* `FLuaFunction` 对Lua侧 `function` 类型的封装，提供获取全局或指定 `table` 内的 `function` 并调用的能力

* `FLuaRetValues` 对从Lua侧的返回值的封装

* `FCheckStack` 用于辅助检测Lua堆栈平衡

* `FAutoStack` 用于自动恢复Lua堆栈索引

* `FExportedEnum` 提供向Lua侧注册导出枚举的接口

## Lua API

### 全局表
* `UE` 仅当 `WITH_UE4_NAMESPACE` 开关启用时存在，作为访问C++侧类的根对象。当开关禁用时，访问C++可以直接通过 `_G` 作为根对象来访问。类型是用到了才会注册到Lua，因此不必担心一次性加载过多导致的性能问题

### 全局函数
* `RegisterEnum` 手动注册一个枚举到Lua

* `RegisterClass` 手动注册一个类到Lua

* `GetUProperty` 获取某个 `UObject` 上的 `UProperty`

* `SetUProperty` 设置某个 `UObject` 的 `UProperty` 为指定值

* `LoadObject` 加载一个 `UObject` ， 相当于：`UObject.Load("/Game/Core/Blueprints/AI/BehaviorTree_Enemy.BehaviorTree_Enemy")`

* `LoadClass` 加载一个 `UClass` ， 相当于： `UClass.Load("/Game/Core/Blueprints/AICharacter.AICharacter_C")`

* `NewObject` 根据指定的Class、Outer(可选)、Name(可选)， 创建一个 `UObject`

* `UEPrint` 是`UE_LOG` 的包装，也会使用 `UKismetSystemLibrary::PrintString` 输出

### 实例函数
* `Initialize` 当任意对象被绑定到Lua时，都会调用这个初始化函数

### 委托
* `Bind` 绑定一个回调到当前 `FScriptDelegate` 实例上。

* `Unbind` 从当前 `FScriptDelegate` 实例上解绑回调。

* `Execute` 手动执行 `FScriptDelegate` 所有回调。

### UnLua.lua
一旦 `require "Unlua"` 后，会产生如下全局函数：
* `Class` 创建一个"类"的table，模仿OOP的概念提供封装、继承的机制

* `print` 使用 `UEPrint` 覆盖Lua原生 `print`

## 宏定义
以下宏定义可通过 `UnLua.Build.cs` 修改：
* `AUTO_UNLUA_STARTUP` 是否自动启动UnLua环境，默认**启用**。

* `WITH_UE4_NAMESPACE` 是否提供一个统一的 `UE` 命名空间 `table` 作为对引擎侧类型的访问入口，默认**启用**。

* `SUPPORTS_RPC_CALL` 是否提供对 `_RPC` 远程方法支持，默认**启用**。

* `SUPPORTS_COMMANDLET` 是否提供在 `commandlet` 环境下的UnLua支持，默认**启用**。

* `ENABLE_TYPE_CHECK` 是否启用类型检测，帮助在 `UFunction` 调用时进行参数类型合法性检测，默认**启用**。

* `UNLUA_ENABLE_DEBUG` 是否输出UnLua详细的调试日志，默认**禁用**。