--[[
    说明：通过绑定 FUnLuaDelegates::CustomLoadLuaFile 可以实现自定义Lua加载器

    演示两种常见的实现供参考：

    方式1：查找路径固定，性能更好
    方式2：通过package.path查找，更加灵活

    本示例C++源码：
    Source\TPSProject\TutorialBlueprintFunctionLibrary.cpp
]]

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

local function print_intro()
    local msg =
        [[

—— 本示例来自 "Content/Script/Tutorials.14_EditorExtension.lua"
]]
    Screen.Print(msg)
end

function M:ReceiveBeginPlay()
    print_intro()

    local UMGClass = UE.UClass.Load("/Game/Tutorials/14_EditorExtension/EditorExtension_UI.EditorExtension_UI_C")
    local Widget = UE.UWidgetBlueprintLibrary.Create(self, UMGClass)
    Widget:AddToViewport()
end

return M
