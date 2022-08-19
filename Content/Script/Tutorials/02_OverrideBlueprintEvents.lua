--[[
    说明：覆盖蓝图事件时，只需要在返回的table中声明 Receive{EventName}

    例如：
    function M:ReceiveBeginPlay()
    end

    除了蓝图事件可以覆盖，也可以直接声明 {FunctionName} 来覆盖Function。
    如果需要调用被覆盖的蓝图Function，可以通过 self.Overridden.{FunctionName}(self, ...) 来访问

    例如：
    function M:SayHi(name)
        self.Overridden.SayHi(self, name)
    end

    注意：这里不可以写成 self.Overridden:SayHi(name)
]] --

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local msg = self:SayHi("陌生人")
    Screen.Print(msg)
end

function M:SayHi(name)
    local origin = self.Overridden.SayHi(self, name)
    return origin .. "\n\n" ..
        [[现在我们已经相互熟悉了，这是来自Lua的问候。

        —— 本示例来自 "Content/Script/Tutorials.02_OverrideBlueprintEvents.lua"
    ]]
end

return M
