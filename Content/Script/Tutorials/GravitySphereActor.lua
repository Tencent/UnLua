require "UnLua"

local FVector = UE4.FVector
local FLinearColor = UE4.FLinearColor

local M = Class()

function M:ReceiveBeginPlay()
    local r = math.random()
    local g = math.random()
    local b = math.random()

    local x = math.random(-20000, 20000)
    local y = math.random(-20000, 20000)
    local z = 100000

    local s = math.random(2, 10) * 0.1

    self.Sphere:CreateDynamicMaterialInstance():SetVectorParameterValue("Color", FLinearColor(r, g, b))
    self.Sphere:SetWorldScale3D(FVector(s, s, s))
    self.Sphere:AddImpulse(FVector(x, y, z))
end

return M
