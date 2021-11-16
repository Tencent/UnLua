require "UnLua"

local BP_CharacterBase_C = Class()

function BP_CharacterBase_C:Initialize(Initializer)
	self.IsDead = false
	self.BodyDuration = 3.0
	self.BoneName = nil
	local Health = 100
	self.Health = Health
	self.MaxHealth = Health
end

--function BP_CharacterBase_C:UserConstructionScript()
--end

function BP_CharacterBase_C:ReceiveBeginPlay()
	local Weapon = self:SpawnWeapon()
	if Weapon then
		Weapon:K2_AttachToComponent(self.WeaponPoint, nil, UE.EAttachmentRule.SnapToTarget, UE.EAttachmentRule.SnapToTarget, UE.EAttachmentRule.SnapToTarget)
		self.Weapon = Weapon
	end
end

function BP_CharacterBase_C:SpawnWeapon()
	return nil
end

function BP_CharacterBase_C:StartFire()
	if self.Weapon then
		self.Weapon:StartFire()
	end
end

function BP_CharacterBase_C:StopFire()
	if self.Weapon then
		self.Weapon:StopFire()
	end
end

function BP_CharacterBase_C:ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser)
	if not self.IsDead then
		local Health = self.Health - Damage
		self.Health = math.max(Health, 0)
		if Health <= 0.0 then
			self:Died(DamageType)
			local co = coroutine.create(BP_CharacterBase_C.Destroy)
			coroutine.resume(co, self, self.BodyDuration)
		end
	end
end

function BP_CharacterBase_C:Died(DamageType)
	self.IsDead = true
	self.CapsuleComponent:SetCollisionEnabled(UE.ECollisionEnabled.NoCollision)
	self:StopFire()
	local Controller = self:GetController()
	Controller:UnPossess()
end

function BP_CharacterBase_C:Destroy(Duration)
	UE.UKismetSystemLibrary.Delay(self, Duration)
	if self.Weapon then
		self.Weapon:K2_DestroyActor()
	end
	self:K2_DestroyActor()
end

function BP_CharacterBase_C:ChangeToRagdoll()
	self.Mesh:SetSimulatePhysics(true)
end

return BP_CharacterBase_C
