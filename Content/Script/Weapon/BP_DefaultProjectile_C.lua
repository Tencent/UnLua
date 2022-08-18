---@type BP_DefaultProjectile_C
local M = UnLua.Class("Weapon.BP_ProjectileBase_C")

function M:Initialize(Initializer)
	if Initializer then
		self.BaseColor = Initializer[0]
	end
end

function M:UserConstructionScript()
	self.Super.UserConstructionScript(self)
	self.DamageType = UE.UClass.Load("/Game/Core/Blueprints/BP_DamageType.BP_DamageType_C")
end

function M:ReceiveBeginPlay()
	self.Super.ReceiveBeginPlay(self)
	local MID = self.StaticMesh:CreateDynamicMaterialInstance(0)
	if MID then
		MID:SetVectorParameterValue("BaseColor", self.BaseColor)
	end
end

return M
