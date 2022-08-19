--[[
    说明：除了实现 UnLuaInterface 的静态绑定方式外，还可以在运行时动态绑定对象到Lua

    对于 Actor 类，可以使用 SpawnActor 接口，例如：
    World:SpawnActor(SpawnClass, Transform, AlwaysSpawn, self, self, "Tutorials.GravitySphereActor")

    对于非 Actor 类，可以使用 NewObject 接口，例如：
    NewObject(WidgetClass, self, nil, "Tutorials.IconWidget")

    注意：无论哪种绑定方式，都需要指定脚本文件路径。它是一个相对于 {工程目录}/Content/Script 下的相对路径。
]]--

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local msg =
        [[
    鼠标左键：创建动态绑定的 Actor

    鼠标右键：创建动态绑定的 Object

    —— 本示例来自 "Content/Script/Tutorials.04_DynamicBinding.lua"
    ]]
    Screen.Print(msg)
end

function M:LeftMouseButton_Pressed()
    local World = self:GetWorld()
    local SpawnClass = self.SpawnClass
    local Transform = self.SpawnPointActor:GetTransform()
    local AlwaysSpawn = UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn
    World:SpawnActor(SpawnClass, Transform, AlwaysSpawn, self, self, "Tutorials.GravitySphereActor")
end

function M:RightMouseButton_Pressed()
    local WidgetClass = self.WidgetClass
    local img = NewObject(WidgetClass, self, nil, "Tutorials.IconWidget")
    img:AddToViewport()
    img:RandomPosition()
end

return M
