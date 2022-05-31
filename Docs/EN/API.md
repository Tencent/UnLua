# UnLua API

## C++ API

### Global Variables
* `UnLua::GLuaSrcRelativePath` Relative path of Lua script source files based on Content directory

* `UnLua::GLuaSrcFullPath` Absolute path of Lua script source files

* `FLuaContext::GLuaCxt` Global Lua context

### Global Functions
Namespace of UnLua
* `Call` Call a global function in Lua.

* `CallTableFunc` Call a function in a given global table in Lua.

* `Get` Return a given type value from the Lua stack at the given inde

* `Push` / `PushXXX` Push a data with given type into the Lua stack

* `IsType` Return the data in Lua stack at given index is the given type or not

* `CreateState` Return global lua_State pointer created in FLuaContext class

* `GetState` Return the global lua_State pointer created by CreateState

* `Startup` Call to start the UnLua, return true for success start, otherwise, return false

* `Shutdown` Call to stop the UnLua

* `LoadFile` Call to load a Lua file with relative path based on Lua Script path without running it, return true if found the file and loaded successfully, otherwise return false

* `RunFile` Call to load and run a Lua file, return true if loaded and run successfully, otherwise return false

* `LoadChunk` Call to load a Lua chunk without running it, return true if loaded successfully, otherwise return false

* `RunChunk` Call to run a Lua chunk, return true if loaded successfully, otherwise return false

* `ReportLuaCallError` Return Lua error, will excute the error delegate if the error delegate was bounded, otherwise will call `UE_LOG` to output the error.

* `GetStackVariables` Get local variables and upvalues of the runtime stack.

* `GetLuaCallStack` Get the call stack of the running lua_State

### UnLua's global Delegates
FUnLuaDelegates
* `OnLuaStateCreated` Trriged after created a lua_State, you can get the lua_State in this delegate

* `OnLuaContextInitialized` Trriged after UnLua's context has been initialized

* `OnPreLuaContextCleanup` Trriged before starting to clean UnLua's context

* `OnPostLuaContextCleanup` Trriged after UnLua's context has been cleaned

* `OnPreStaticallyExport` Trriged before starting to register the statically exported classes into lua_State

* `OnObjectBinded` Trriged after an UObject has been binned with a lua file

* `OnObjectUnbinded` Trriged after an UObject has been unbinned with a lua file

* `ReportLuaCallError` Trriged after Lua VM occured an error, could register user-defined implementation

* `ConfigureLuaGC` Trriged after lua_State was created, could register user-defined lua gc configuration

* `LoadLuaFile` Trriged while try to loading lua file, could register user-defined loading

### Interface
* `UUnLuaInterface` Core interface for UnLua, used to tell UnLua which UE-Object should be bind with Lua file

### Class
* `FLuaContext` Wrapper of UnLua's context, contains the main lua_State of UnLua

* `FLuaIndex` Wrapper of the index of Lua stack

* `FLuaTable` Wrapper of Lua table, could get tables's data by `[]`, and also support call a function

* `FLuaValue` Wrapper of generic Lua value

* `FLuaFunction` Wrapper of Lua function, could be global function or funciton in global table

* `FLuaRetValues` Wrapper of Lua funciton's return value

* `FCheckStack` Helper for checking Lua stack is correct or not

* `FAutoStack` Helper to recover Lua stack automatically

* `FExportedEnum` Interface for exporting enum into lua

## Lua API

### Global Tables
* `UE` Could use `UE.UEClass` to access class in c++ if you open `WITH_UE4_NAMESPACE`, otherwise, could access class in c++ by `_G.UEClass`. Class will be registered into Lua while being used, so don’t worry about the performance problems caused by too much loading at one time.

### Global Functions
* `RegisterEnum` Used to manually register an enum into Lua

* `RegisterClass` Used to manually register a class into Lua

* `GetUProperty` Return the UProperty of given UObject by given name

* `SetUProperty` Set the value of given UProperty in given UObject with given value

* `LoadObject` Loads an UObject, same as `UObject.Load("/Game/Core/Blueprints/AI/BehaviorTree_Enemy.BehaviorTree_Enemy`

* `LoadClass` Loads a UClass, same as `UClass.Load("/Game/Core/Blueprints/AICharacter.AICharacter_C`

* `NewObject` Creates an UObject with given Class and Outer(Optional) and Name(Opthional)

* `UEPrint` Wrapper of `UE_LOG`, internally use `UKismetSystemLibrary::PrintString`

### Functions of Instances
* `Initialize` Assuming that every Lua Table has this function, it will be called when binding with UObject

### Functions for UE Delegate
* `Bind` Binds the given lua callback with the given instance of FScriptDelegate

* `Unbind` Unbinds the lua callback of the given instance of FScriptDelegate

* `Execute` Manually excutes all callbacks of the given instance of FScriptDelegate

### UnLua.lua (Functions in UnLua.lua)
* `Class` Create a table like class in OOP

* `print` Override origin `print` by `UEPrint`

## Macros in UnLua module
* `AUTO_UNLUA_STARTUP` Macro for whether to automatically start or not, default is true

* `WITH_UE4_NAMESPACE` Macro for whether to use `UE` to access Unreal Class's in Lua or not , default is true

* `SUPPORTS_RPC_CALL` Macro for whether to support RPC calls, default is true

* `SUPPORTS_COMMANDLET` Macro for whether to support UnLua in `commandlet`, default is true

* `ENABLE_TYPE_CHECK` Macro for whether to check arguments' type is correct or not while calling C++ from Lua, default is true

* `UNLUA_ENABLE_DEBUG` Macro for whether to output UnLua's debugging log, default is true.