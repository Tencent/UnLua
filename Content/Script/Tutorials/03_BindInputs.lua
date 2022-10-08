--[[
    说明：需要监听按键或Action时，只需要在返回的table中声明 {KeyName}_Pressed / {KeyName}_Released

    例如：
    function M:SpaceBar_Pressed()
    end

    {KeyName}可以参考EKeys的文档：
    https://docs.unrealengine.com/4.26/en-US/API/Runtime/InputCore/EKeys/
    
    或者源码：
    \Engine\Source\Runtime\InputCore\Classes\InputCoreTypes.h
]]--

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local msg =
        [[
    来试试以下输入吧：

    字母、数字、小键盘、方向键、鼠标

    —— 本示例来自 "Content/Script/Tutorials.03_BindInputs.lua"
    ]]
    Screen.Print(msg)
end

local function SetupKeyBindings()
    local key_names = {
        -- 字母
        "A", "B", --[["C",]] "D", "E","F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", --[["V", ]] "W", "X", "Y", "Z",
        -- 数字
        "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine",
        -- 小键盘
        "NumPadOne", "NumPadTwo", "NumPadThree", "NumPadFour", "NumPadFive", "NumPadSix", "NumPadSeven", "NumPadEight", "NumPadNine",
        -- 方向键
        "Up", "Down", "Left", "Right",
        -- ProjectSettings -> Engine - Input -> Action Mappings
        "Fire", "Aim",
    }
    
    for _, key_name in ipairs(key_names) do
        M[key_name .. "_Pressed"] = function(self, key)
            Screen.Print(string.format("按下了%s", key.KeyName))
        end
    end
end

local function SetupAxisBindings()
    -- ProjectSettings -> Engine - Input -> Axis Mappings
    local axis_names = {
        "MoveForward", "MoveRight", "Turn", "LookUp", "LookUpRate", "TurnRate"
    }
    for _, axis_name in ipairs(axis_names) do
        M[axis_name] = function(self, value)
            if value ~= 0 then
                Screen.Print(string.format("%s(%f)", axis_name, value))
            end
        end
    end
end

SetupKeyBindings()
SetupAxisBindings()

--[[
    使用UnLua.Input.BindXXX接口可以实现更细节的输入绑定控制

    更多请参考：
    UnLua\Plugins\UnLua\Content\Script\UnLua\Input.lua
]]

local BindKey = UnLua.Input.BindKey

BindKey(M, "C", "Pressed", function(self, Key)
    Screen.Print("按下了C")
end)

BindKey(M, "C", "Pressed", function(self, Key)
    Screen.Print("复制")
end, { Ctrl = true })

BindKey(M, "V", "Pressed", function(self, Key)
    Screen.Print("按下了V")
end)

BindKey(M, "V", "Pressed", function(self, Key)
    Screen.Print("粘贴")
end, { Ctrl = true })

return M
