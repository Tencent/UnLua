--[[
    说明：如果需要从C++侧调用Lua，需要将UnLua模块添加到 {工程名}.Build.cs 的依赖配置里

    如果需要访问Lua原生API，则还需要添加Lua模块

    例如：
    PrivateDependencyModuleNames.AddRange(new string[]
    {
        "UnLua",
        "Lua",
    });

    本示例C++源码：
    Source\TPSProject\TutorialBlueprintFunctionLibrary.cpp
]]--

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local msg =
        [[

    —— 本示例来自 "Content/Script/Tutorials.08_CppCallLua.lua"
    ]]
    Screen.Print(msg)
    UE.UTutorialBlueprintFunctionLibrary.CallLuaByGlobalTable()
    Screen.Print("=================")
    UE.UTutorialBlueprintFunctionLibrary.CallLuaByFLuaTable()
end

function M.CallMe(a, b)
    local ret = a + b
    local msg = string.format("[Lua]收到来自C++的调用，a=%f b=%f，返回%f", a, b, ret)
    Screen.Print(msg)
    return ret
end

return M
