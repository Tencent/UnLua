--[[
    说明：在Lua协程中可以方便的使用UE4的Latent函数实现延迟执行的效果
]] --

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

local function run(self, name)
    Screen.Print(string.format("协程%s：启动", name))
    for i = 1, 5 do
        UE.UKismetSystemLibrary.Delay(self, 1)
        Screen.Print(string.format("协程%s：%d", name, i))
    end
    Screen.Print(string.format("协程%s：结束", name))
end

function M:ReceiveBeginPlay()
    local msg = [[
    —— 本示例来自 "Content/Script/Tutorials.07_CallLatentFunction.lua"
    ]]
    Screen.Print(msg)

    coroutine.resume(coroutine.create(run), self, "A")
    coroutine.resume(coroutine.create(run), self, "B")
end

return M
