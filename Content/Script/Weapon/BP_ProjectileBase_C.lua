require "UnLua"

local BP_ProjectileBase_C = Class()

function BP_ProjectileBase_C:UserConstructionScript()
	self.Damage = 128.0
	self.DamageType = nil
	self.Sphere.OnComponentHit:Add(self, BP_ProjectileBase_C.OnComponentHit_Sphere)
end

function BP_ProjectileBase_C:ReceiveBeginPlay()
	self:SetLifeSpan(4.0)
end

function BP_ProjectileBase_C:OnComponentHit_Sphere(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit)
	local Character = OtherActor:Cast(UE.ABP_CharacterBase_C)
	if Character then
		Character.BoneName = Hit.BoneName;
		local Controller = self.Instigator:GetController()
		UE.UGameplayStatics.ApplyDamage(Character, self.Damage, Controller, self.Instigator, self.DamageType)
	end
	self:K2_DestroyActor()
end

function BP_ProjectileBase_C:ReceiveDestroyed()
	--self.Sphere.OnComponentHit:Remove(self, BP_ProjectileBase_C.OnComponentHit_Sphere)
end

return BP_ProjectileBase_C
