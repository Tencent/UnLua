--[[
    说明：当勾选了 UnLuaInterface 接口上的 Run in Editor 选项后，Lua绑定会在编辑器非运行游戏时（Play in Editor）也会生效

    利用这点，我们可以非常方便地使用Lua来扩展编辑器功能

    小提示：
    1. 放在 Content/Editor 目录下的资源，会默认勾选 Run in Editor 选项
    2. 编辑器启动时会自动执行 Content/Script/Editor/Main.lua （若文件存在）
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
