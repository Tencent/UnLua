---@type BP_TriggerArea_C
local M = UnLua.Class()

local function update(self)
    if self.Counter > 0 then
        self.PlaneMaterial:SetVectorParameterValue("Color", self.TriggerColor)
    else
        self.PlaneMaterial:SetVectorParameterValue("Color", self.NormalColor)
    end
end

function M:ReceiveBeginPlay()
    self.Counter = 0
    self.PlaneMaterial = self.Plane:CreateDynamicMaterialInstance(0)
    update(self)
end

function M:ReceiveActorBeginOverlap(OtherActor)
    self.Counter = self.Counter + 1
    update(self)
end

function M:ReceiveActorEndOverlap(OtherActor)
    self.Counter = self.Counter - 1
    update(self)
end

return M