---@type BP_WeaponBase_C
local M = UnLua.Class()

local EFireType = {	FT_Projectile = 0, FT_InstantHit = 1 }

function M:UserConstructionScript()
	self.IsFiring = false
	self.InfiniteAmmo = false
	self.FireInterval = 0.2
	self.MaxAmmo = 30
	self.AmmoPerShot = 1
	self.FireType = EFireType.FT_Projectile
	self.WeaponTraceDistance = 100000.0
	self.MuzzleSocketName = nil
	self.AimingFOV = 45.0
end

function M:ReceiveBeginPlay()
	self.CurrentAmmo = self.MaxAmmo
end

function M:StartFire()
	self.IsFiring = true
	self:FireAmmunition()
	self.TimerHandle = UE.UKismetSystemLibrary.K2_SetTimerDelegate({self, M.Refire}, self.FireInterval, true)
end

function M:StopFire()
	if self.IsFiring then
		self.IsFiring = false
		UE.UKismetSystemLibrary.K2_ClearTimerHandle(self, self.TimerHandle)
	end
end

function M:FireAmmunition()
	self:ConsumeAmmo()
	self:PlayWeaponAnimation()
	self:PlayMuzzleEffect()
	self:PlayFireSound()
	if self.FireType == EFireType.FT_Projectile then
		self:ProjectileFire()
	else
		self:InstantFire()
	end
end

function M:ConsumeAmmo()
	if not self.InfiniteAmmo then
		local Ammo = self.CurrentAmmo - self.AmmoPerShot
		self.CurrentAmmo = math.max(Ammo, 0)
	end
end

function M:PlayWeaponAnimation()
end

function M:PlayMuzzleEffect()
end

function M:PlayFireSound()
end

function M:ProjectileFire()
	self:SpawnProjectile()
end

function M:SpawnProjectile()
	return nil
end

function M:InstantFire()
	local Transform = self:GetFireInfo()
	local Start = Transform.Translation
	local ForwardVector = Transform.Rotation:GetForwardVector()
	local End = ForwardVector * self.WeaponTraceDistance
	End.Add(Start)
	--local HitResult = UE.FHitResult()
	--local ActorsToIgnore = TArray(AActor)
	local bResult = UE.UKismetSystemLibrary.LineTraceSingle(self, Start, End, UE.ETraceTypeQuery.Weapon, false, nil, UE.EDrawDebugTrace.None, nil, true)
	if bResult then
		-- todo:
	end
end

function M:GetFireInfo()
	local UBPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")
	local TraceStart, TraceDirection = UBPI_Interfaces.GetWeaponTraceInfo(self.Instigator)
	local Delta = TraceDirection * self.WeaponTraceDistance
	local TraceEnd = TraceStart + Delta
	local HitResult = UE.FHitResult()
	--local ActorsToIgnore = TArray(AActor)
	local bResult = UE.UKismetSystemLibrary.LineTraceSingle(self, TraceStart, TraceEnd, UE.ETraceTypeQuery.Weapon, false, nil, UE.EDrawDebugTrace.None, HitResult, true)
	local Translation = self.SkeletalMesh:GetSocketLocation(self.MuzzleSocketName)
	local Rotation
	if bResult then
		local ImpactPoint = HitResult.ImpactPoint
		Rotation = UE.UKismetMathLibrary.FindLookAtRotation(Translation, ImpactPoint)
	else
		Rotation = UE.UKismetMathLibrary.FindLookAtRotation(Translation, TraceEnd)
	end
	local Transform = UE.FTransform(Rotation:ToQuat(), Translation)
	return Transform
end

function M:Refire()
	local bHasAmmo = self:HasAmmo()
	if bHasAmmo and self.IsFiring then
		self:FireAmmunition()
	end
end

function M:HasAmmo()
	return self.InfiniteAmmo or self.CurrentAmmo > 0
end

return M
