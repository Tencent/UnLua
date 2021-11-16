require "UnLua"

local BP_DefaultProjectile_C = Class("Weapon.BP_ProjectileBase_C")

function BP_DefaultProjectile_C:Initialize(Initializer)
	if Initializer then
		self.BaseColor = Initializer[0]
	end
end

function BP_DefaultProjectile_C:UserConstructionScript()
	self.Super.UserConstructionScript(self)
	self.DamageType = UE.UClass.Load("/Game/Core/Blueprints/BP_DamageType.BP_DamageType_C")
end

function BP_DefaultProjectile_C:ReceiveBeginPlay()
	self.Super.ReceiveBeginPlay(self)
	local MID = self.StaticMesh:CreateDynamicMaterialInstance(0)
	if MID then
		MID:SetVectorParameterValue("BaseColor", self.BaseColor)
	end
end

return BP_DefaultProjectile_C
