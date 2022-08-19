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

—— 本示例来自 "Content/Script/Tutorials.12_CustomLoader.lua"
]]
    Screen.Print(msg)
end

function M:ReceiveBeginPlay()
    print_intro()

    UE.UTutorialBlueprintFunctionLibrary.SetupCustomLoader(1)
    Screen.Print(string.format("FromCustomLoader1:%s", require("Tutorials")))
    
    package.loaded["Tutorials"] = nil

    package.path = package.path .. ";./?/Index.lua"
    UE.UTutorialBlueprintFunctionLibrary.SetupCustomLoader(2)
    Screen.Print(string.format("FromCustomLoader2:%s", require("Tutorials")))

    UE.UTutorialBlueprintFunctionLibrary.SetupCustomLoader(0)
end

return M
