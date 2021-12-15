require "UnLua"

local BP_WeaponBase_C = Class()

local EFireType = {	FT_Projectile = 0, FT_InstantHit = 1 }

function BP_WeaponBase_C:UserConstructionScript()
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

function BP_WeaponBase_C:ReceiveBeginPlay()
	self.CurrentAmmo = self.MaxAmmo
end

function BP_WeaponBase_C:StartFire()
	self.IsFiring = true
	self:FireAmmunition()
	self.TimerHandle = UE.UKismetSystemLibrary.K2_SetTimerDelegate({self, BP_WeaponBase_C.Refire}, self.FireInterval, true)
end

function BP_WeaponBase_C:StopFire()
	if self.IsFiring then
		self.IsFiring = false
		UE.UKismetSystemLibrary.K2_ClearTimerHandle(self, self.TimerHandle)
	end
end

function BP_WeaponBase_C:FireAmmunition()
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

function BP_WeaponBase_C:ConsumeAmmo()
	if not self.InfiniteAmmo then
		local Ammo = self.CurrentAmmo - self.AmmoPerShot
		self.CurrentAmmo = math.max(Ammo, 0)
	end
end

function BP_WeaponBase_C:PlayWeaponAnimation()
end

function BP_WeaponBase_C:PlayMuzzleEffect()
end

function BP_WeaponBase_C:PlayFireSound()
end

function BP_WeaponBase_C:ProjectileFire()
	self:SpawnProjectile()
end

function BP_WeaponBase_C:SpawnProjectile()
	return nil
end

function BP_WeaponBase_C:InstantFire()
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

function BP_WeaponBase_C:GetFireInfo()
	--[[
	local TraceStart = FVector()
	local TraceDirection = FVector()
	UE.UBPI_Interfaces_C.GetWeaponTraceInfo(self.Instigator, TraceStart, TraceDirection)
	]]
	local TraceStart, TraceDirection = UE.UBPI_Interfaces_C.GetWeaponTraceInfo(self.Instigator)
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

function BP_WeaponBase_C:Refire()
	local bHasAmmo = self:HasAmmo()
	if bHasAmmo and self.IsFiring then
		self:FireAmmunition()
	end
end

function BP_WeaponBase_C:HasAmmo()
	return self.InfiniteAmmo or self.CurrentAmmo > 0
end

return BP_WeaponBase_C
