---@type BP_CharacterBase_C
local M = UnLua.Class()

function M:Initialize(Initializer)
	self.IsDead = false
	self.BodyDuration = 3.0
	self.BoneName = nil
	local Health = 100
	self.Health = Health
	self.MaxHealth = Health
end

--function M:UserConstructionScript()
--end

function M:ReceiveBeginPlay()
	local Weapon = self:SpawnWeapon()
	if Weapon then
		Weapon:K2_AttachToComponent(self.WeaponPoint, nil, UE.EAttachmentRule.SnapToTarget, UE.EAttachmentRule.SnapToTarget, UE.EAttachmentRule.SnapToTarget)
		self.Weapon = Weapon
	end
end

function M:SpawnWeapon()
	return nil
end

function M:StartFire_Server_RPC()
	self:StartFire_Multicast()
end

function M:StartFire_Multicast_RPC()
	if self.Weapon then
		self.Weapon:StartFire()
	end
end

function M:StopFire_Server_RPC()
	self:StopFire_Multicast()
end

function M:StopFire_Multicast_RPC()
	if self.Weapon then
		self.Weapon:StopFire()
	end
end

function M:ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser)
	if not self.IsDead then
		local Health = self.Health - Damage
		self.Health = math.max(Health, 0)
		if Health <= 0.0 then
			self:Died_Multicast(DamageType)
			local co = coroutine.create(M.Destroy)
			coroutine.resume(co, self, self.BodyDuration)
		end
	end
end

function M:Died_Multicast_RPC(DamageType)
	self.IsDead = true
	self.CapsuleComponent:SetCollisionEnabled(UE.ECollisionEnabled.NoCollision)
	self:StopFire()
	local Controller = self:GetController()
	if Controller then
		Controller:UnPossess()
	end
end

function M:Destroy(Duration)
	UE.UKismetSystemLibrary.Delay(self, Duration)
	if not self:IsValid() then
		return false
	end

	if self.Weapon then
		self.Weapon:K2_DestroyActor()
	end
	self:K2_DestroyActor()
end

function M:ChangeToRagdoll()
	self.Mesh:SetSimulatePhysics(true)
end

return M
