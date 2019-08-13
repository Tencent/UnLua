require "UnLua"

local BP_DefaultWeapon_C = Class("Weapon.BP_WeaponBase_C")

function BP_DefaultWeapon_C:UserConstructionScript()
	self.Super.UserConstructionScript(self)
	self.InfiniteAmmo = true
	self.ProjectileClass = UE4.UClass.Load("/Game/Core/Blueprints/Weapon/BP_DefaultProjectile")
	self.MuzzleSocketName = "Muzzle"
	self.World = self:GetWorld()
end

function BP_DefaultWeapon_C:SpawnProjectile()
	if self.ProjectileClass then
		local Transform = self:GetFireInfo()
		local R = UE4.UKismetMathLibrary.RandomFloat()
		local G = UE4.UKismetMathLibrary.RandomFloat()
		local B = UE4.UKismetMathLibrary.RandomFloat()
		local BaseColor = {}
		BaseColor[0] = UE4.FLinearColor(R, G, B, 1.0)
		local Projectile = self.World:SpawnActor(self.ProjectileClass, Transform, UE4.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self, self.Instigator, "Weapon.BP_DefaultProjectile_C", BaseColor)
	end
end

return BP_DefaultWeapon_C
