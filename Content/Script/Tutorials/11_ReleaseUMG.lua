--[[
    说明：

    UMG对象的释放流程：
    1、调用self:Release()，解除LuaTable在C++侧的引用
    2、确保LuaTable在Lua侧没有其他引用，触发LuaGC
    3、C++侧收到UObject_Delete回调，解除UMG在C++侧的引用
    4、确保UMG在C++侧没有其他引用，触发UEGC

    小提示：

    使用控制台命令查看对象和类的引用情况：
    
    查看指定类的引用列表：Obj List Class=ReleaseUMG_Root_C
    查看指定对象的引用链：Obj Refs Name=ReleaseUMG_Root_C_0
]] --

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

local function print_intro()
    local msg =
        [[
使用以下按键进行一次强制垃圾回收：

C：强制 C++ GC
L：强制 Lua GC

—— 本示例来自 "Content/Script/Tutorials.11_ReleaseUMG.lua"
]]
    Screen.Print(msg)
end

function M:ReceiveBeginPlay()
    local widget_class = UE.UClass.Load("/Game/Tutorials/11_ReleaseUMG/ReleaseUMG_Root.ReleaseUMG_Root_C")
    local widget_root = NewObject(widget_class, self)
    widget_root:AddToViewport()

    print_intro()
end

function M:L_Pressed()
    collectgarbage("collect")
    Screen.Print('collectgarbage("collect")')
end

function M:C_Pressed()
    UE.UKismetSystemLibrary.CollectGarbage()
    Screen.Print("UKismetSystemLibrary.CollectGarbage()")
end

return M
