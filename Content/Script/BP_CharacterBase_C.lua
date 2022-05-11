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

function BP_CharacterBase_C:StartFire_Server_RPC()
	self:StartFire_Multicast()
end

function BP_CharacterBase_C:StartFire_Multicast_RPC()
	if self.Weapon then
		self.Weapon:StartFire()
	end
end

function BP_CharacterBase_C:StopFire_Server_RPC()
	self:StopFire_Multicast()
end

function BP_CharacterBase_C:StopFire_Multicast_RPC()
	if self.Weapon then
		self.Weapon:StopFire()
	end
end

function BP_CharacterBase_C:ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser)
	if not self.IsDead then
		local Health = self.Health - Damage
		self.Health = math.max(Health, 0)
		if Health <= 0.0 then
			self:Died_Multicast(DamageType)
			local co = coroutine.create(BP_CharacterBase_C.Destroy)
			coroutine.resume(co, self, self.BodyDuration)
		end
	end
end

function BP_CharacterBase_C:Died_Multicast_RPC(DamageType)
	self.IsDead = true
	self.CapsuleComponent:SetCollisionEnabled(UE.ECollisionEnabled.NoCollision)
	self:StopFire()
	local Controller = self:GetController()
	if Controller then
		Controller:UnPossess()
	end
end

function BP_CharacterBase_C:Destroy(Duration)
	UE.UKismetSystemLibrary.Delay(self, Duration)
	if not self:IsValid() then
		return false
	end

	if self.Weapon then
		self.Weapon:K2_DestroyActor()
	end
	self:K2_DestroyActor()
end

function BP_CharacterBase_C:ChangeToRagdoll()
	self.Mesh:SetSimulatePhysics(true)
end

return BP_CharacterBase_C
