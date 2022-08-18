local Screen = require "Tutorials.Screen"
local FVector2D = UE.FVector2D
local FLinearColor = UE.FLinearColor

---@type IconWidget_C
local M = UnLua.Class()

function M:RandomPosition()
    local x = math.random(0, 1920)
    local y = math.random(0, 960)
    self:SetPositionInViewport(FVector2D(x, y))
end

return M
