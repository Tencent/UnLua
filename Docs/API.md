# UnLua API

## C++ API

### 全局变量(Global Variables)
* `UnLua::GLuaSrcRelativePath` Lua源码文件相对于Content目录的相对路径 (Relative path of Lua script source files based on Content directory)

* `UnLua::GLuaSrcFullPath` Lua源码文件所在的绝对路径 (Absolute path of Lua script source files)

* `FLuaContext::GLuaCxt` 全局Lua上下文 (Global Lua context)

### 全局函数(Global Functions)
命名空间：UnLua (Namespace of UnLua)
* `Call` 调用某个全局 `function` (Call a global function in Lua.)

* `CallTableFunc` 调用全局 `table` 里的某个 `function` (Call a function in a given global table in Lua.)

* `Get` 从Lua堆栈的某个索引位置上获取指定类型的数据 (Return a given type value from the Lua stack at the given index)

* `Push` / PushXXX 向Lua堆栈中压入一个指定类型的数据 (Push a data with given type into the Lua stack)

* `IsType` 判断Lua堆栈上指定索引位置上的对象类型 (Return the data in Lua stack at given index is the given type or not)

* `CreateState` 从 `GLuaCxt` 创建一个 `lua_State` (Return global lua_State pointer created in FLuaContext class)

* `GetState` 获取 `GLuaCxt` 中的 `lua_State` (Return the global lua_State pointer created by CreateState)

* `Startup` 启动UnLua环境 (Call to start the UnLua, return true for success start, otherwise, return false)

* `Shutdown` 停止UnLua环境 (Call to stop the UnLua)

* `LoadFile` 从指定相对路径加载一个Lua文件，但不运行 (Call to load a Lua file with relative path based on Lua Script path without running it, return true if found the file and loaded successfully, otherwise return false)

* `RunFile` 从指定相对路径运行一个Lua文件 (Call to load and run a Lua file, return true if loaded and run successfully, otherwise return false)

* `LoadChunk` 加载一个Lua Chunk，但不运行 (Call to load a Lua chunk without running it, return true if loaded successfully, otherwise return false)

* `RunChunk` 运行指定的Lua Chunk (Call to run a Lua chunk, return true if loaded successfully, otherwise return false)

* `ReportLuaCallError` 报告Lua错误，如果注册了自定义错误处理委托则会仅执行委托，否则使用UnLua默认的 `UE_LOG` 输出 (Return Lua error, will excute the error delegate if the error delegate was bounded, otherwise will call `UE_LOG` to output the error.)

* `GetStackVariables` 获取当前堆栈上所有相关的变量、`upvalue` 信息 (Get local variables and upvalues of the runtime stack.)

* `GetLuaCallStack` 获取当前Lua调用栈的字符串 (Get the call stack of the running lua_State)

### 全局委托(UnLua's global Delegates)
FUnLuaDelegates
* `OnLuaStateCreated` 当一个Lua虚拟机被创建出来时候触发，可以拿到其对应的 `lua_State` (Trriged after created a lua_State, you can get the lua_State in this delegate)

* `OnLuaContextInitialized` 当 `FLuaContext` 初始化完成后触发 (Trriged after UnLua's context has been initialized)

* `OnPreLuaContextCleanup` 当 `FLuaContext` 开始回收清理前触发 (Trriged before starting to clean UnLua's context)

* `OnPostLuaContextCleanup` 当 `FLuaContext` 完成回收清理后触发 (Trriged after UnLua's context has been cleaned)

* `OnPreStaticallyExport` 当 `FLuaContext` 开始向 `lua_State` 注册静态导出的类型之前触发 (Trriged before starting to register the statically exported classes into lua_State)

* `OnObjectBinded` 当某个 `UObject` 和Lua绑定时触发 (Trriged after an UObject has been binned with a lua file)

* `OnObjectUnbinded` 当某个 `UObject` 的Lua绑定被解绑时触发 (Trriged after an UObject has been unbinned with a lua file)

* `ReportLuaCallError` 支持注册自定义Lua报错处理委托到UnLua，当Lua报错时会调用这个委托 (Trriged after Lua VM occured an error, could register user-defined implementation)

* `ConfigureLuaGC` 支持注册自定义LuaGC配置委托到UnLua，覆盖UnLua默认传递给 `lua_gc` 的参数 (Trriged after lua_State was created, could register user-defined lua gc configuration)

* `LoadLuaFile` 支持注册自定义的Lua文件加载器委托到UnLua，实现自定义Lua文件加载机制 (Trriged while try to loading lua file, could register user-defined loading)

### 接口(Interface)
* `UUnLuaInterface` UnLua的核心接口，用来标记和识别需要绑定到Lua的对象 (Core interface for UnLua, used to tell UnLua which UE-Object should be bind with Lua file)

### 类
* `FLuaContext` 封装了整个UnLua运行环境上下文 (Wrapper of UnLua's context, contains the main lua_State of UnLua)

* `FLuaIndex` 对Lua堆栈索引的简单封装 (Wrapper of the index of Lua stack)

* `FLuaTable` 对Lua侧 `table` 数据类型的封装，提供类似Lua中"`[]`"的访问机制，也支持调用指定 `function`  (Wrapper of Lua table, could get tables's data by `[]`, and also support call a function)

* `FLuaValue` 对Lua各种值类型的封装 (Wrapper of generic Lua value)

* `FLuaFunction` 对Lua侧 `function` 类型的封装，提供获取全局或指定 `table` 内的 `function` 并调用的能力 (Wrapper of Lua function, could be global function or funciton in global table)

* `FLuaRetValues` 对从Lua侧的返回值的封装 (Wrapper of Lua funciton's return value)

* `FCheckStack` 用于辅助检测Lua堆栈平衡 (Helper for checking Lua stack is correct or not)

* `FAutoStack` 用于自动恢复Lua堆栈索引 (Helper to recover Lua stack automatically)

* `FExportedEnum` 提供向Lua侧注册导出枚举的接口 (Interface for exporting enum into lua)

## Lua API

### 全局表(Global Tables)
* `UE` 仅当 `WITH_UE4_NAMESPACE` 开关启用时存在，作为访问C++侧类的根对象。当开关禁用时，访问C++可以直接通过 `_G` 作为根对象来访问。类型是用到了才会注册到Lua，因此不必担心一次性加载过多导致的性能问题 (Could use `UE.UEClass` to access class in c++ if you open `WITH_UE4_NAMESPACE`, otherwise, could access class in c++ by `_G.UEClass`. Class will be registered into Lua while being used, so don’t worry about the performance problems caused by too much loading at one time.)

### 全局函数(Global Functions)
* `RegisterEnum` 手动注册一个枚举到Lua (Used to manually register an enum into Lua)

* `RegisterClass` 手动注册一个类到Lua (Used to manually register a class into Lua)

* `GetUProperty` 获取某个 `UObject` 上的 `UProperty`  (Return the UProperty of given UObject by given name)

* `SetUProperty` 设置某个 `UObject` 的 `UProperty` 为指定值 (Set the value of given UProperty in given UObject with given value)

* `LoadObject` 加载一个 `UObject` ， 相当于：`UObject.Load("/Game/Core/Blueprints/AI/BehaviorTree_Enemy.BehaviorTree_Enemy")` (Loads an UObject, same as `UObject.Load("/Game/Core/Blueprints/AI/BehaviorTree_Enemy.BehaviorTree_Enemy`)

* `LoadClass` 加载一个 `UClass` ， 相当于： `UClass.Load("/Game/Core/Blueprints/AICharacter.AICharacter_C")` (Loads a UClass, same as `UClass.Load("/Game/Core/Blueprints/AICharacter.AICharacter_C`)

* `NewObject` 根据指定的Class、Outer(可选)、Name(可选)， 创建一个 `UObject` (Creates an UObject with given Class and Outer(Optional) and Name(Opthional))

* `UEPrint` 是`UE_LOG` 的包装，也会使用 `UKismetSystemLibrary::PrintString` 输出 (Wrapper of `UE_LOG`, internally use `UKismetSystemLibrary::PrintString`)

### 实例函数(Functions of Instances)
* `Initialize` 当任意对象被绑定到Lua时，都会调用这个初始化函数 (Assuming that every Lua Table has this function, it will be called when binding with UObject)

### 委托(Functions for UE Delegate)
* `Bind` 绑定一个回调到当前 `FScriptDelegate` 实例上。(Binds the given lua callback with the given instance of FScriptDelegate)

* `Unbind` 从当前 `FScriptDelegate` 实例上解绑回调。(Unbinds the lua callback of the given instance of FScriptDelegate)

* `Execute` 手动执行 `FScriptDelegate` 所有回调。(Manually excutes all callbacks of the given instance of FScriptDelegate)

### UnLua.lua (Functions in UnLua.lua)
一旦 `require "Unlua"` 后，会产生如下全局函数：
* `Class` 创建一个"类"的table，模仿OOP的概念提供封装、继承的机制 (Create a table like class in OOP)

* `print` 使用 `UEPrint` 覆盖Lua原生 `print` (Override origin `print` by `UEPrint`)

## 宏定义(Macros in UnLua module)
以下宏定义可通过 `UnLua.Build.cs` 修改：
* `AUTO_UNLUA_STARTUP` 是否自动启动UnLua环境，默认**启用**。 (Macro for whether to automatically start or not, default is true)

* `WITH_UE4_NAMESPACE` 是否提供一个统一的 `UE` 命名空间 `table` 作为对引擎侧类型的访问入口，默认**启用**。(Macro for whether to use `UE` to access Unreal Class's in Lua or not , default is true)

* `SUPPORTS_RPC_CALL` 是否提供对 `_RPC` 远程方法支持，默认**启用**。(Macro for whether to support RPC calls, default is true)

* `SUPPORTS_COMMANDLET` 是否提供在 `commandlet` 环境下的UnLua支持，默认**启用**。(Macro for whether to support UnLua in `commandlet`, default is true)

* `ENABLE_TYPE_CHECK` 是否启用类型检测，帮助在 `UFunction` 调用时进行参数类型合法性检测，默认**启用**。(Macro for whether to check arguments' type is correct or not while calling C++ from Lua, default is true)

* `UNLUA_ENABLE_DEBUG` 是否输出UnLua详细的调试日志，默认**禁用**。(Macro for whether to output UnLua's debugging log, default is true.)