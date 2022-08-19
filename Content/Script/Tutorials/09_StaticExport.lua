--[[
    说明：

    本示例C++源码：
    \Source\TPSProject\TutorialObject.cpp
]]--

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local msg =
        [[

    —— 本示例来自 "Content/Script/Tutorials.09_StaticExport.lua"
    ]]
    Screen.Print(msg)
    
    local tutorial = UE.FTutorialObject("教程")
    msg = string.format("tutorial -> %s\n\ntutorial:GetTitle() -> %s", tostring(tutorial), tutorial:GetTitle())
    Screen.Print(msg)
end

return M
