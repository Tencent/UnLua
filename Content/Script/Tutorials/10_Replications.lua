--[[
    说明：
    
    使用 {函数名}_RPC 可以覆盖蓝图中RPC函数的实现
    使用 OnRep_{变量名} 可以覆盖蓝图中变量同步消息的处理

    蓝图示例：
    Content/Tutorials/10_Replications/ChatCharacter.uasset

    脚本示例：
    Content/Script/Tutorials/ChatCharacter.lua
]] --

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    if not self:HasAuthority() then
        return
    end
    local msg = [[

    试试使用多人模式运行示例吧

    方向键：移动
    空格：跳跃

    —— 本示例来自 "Content/Script/Tutorials.10_Replications.lua"
    ]]
    Screen.Print(msg)
end

return M
