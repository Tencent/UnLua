--[[
    说明：在蓝图中实现UnLuaInterface接口，并通过 GetModuleName 指定脚本路径，即可绑定到Lua

    例如：
    本脚本由 "Content/Tutorials/01_HelloWorld/HelloWorld.map" 的关卡蓝图绑定
]]

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

-- 所有绑定到Lua的对象初始化时都会调用Initialize的实例方法
function M:Initialize()
    local msg = [[
    Hello World!

    —— 本示例来自 "Content/Script/Tutorials/01_HelloWorld.lua"
    ]]
    print(msg)
    Screen.Print(msg)
end

return M
