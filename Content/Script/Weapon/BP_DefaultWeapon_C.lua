require "UnLua"

local BP_DefaultWeapon_C = Class("Weapon.BP_WeaponBase_C")

function BP_DefaultWeapon_C:UserConstructionScript()
	self.Super.UserConstructionScript(self)
	self.InfiniteAmmo = true
	self.ProjectileClass = UE.UClass.Load("/Game/Core/Blueprints/Weapon/BP_DefaultProjectile")
	self.MuzzleSocketName = "Muzzle"
	self.World = self:GetWorld()
end

function BP_DefaultWeapon_C:SpawnProjectile()
	if self.ProjectileClass then
		local Transform = self:GetFireInfo()
		local R = UE.UKismetMathLibrary.RandomFloat()
		local G = UE.UKismetMathLibrary.RandomFloat()
		local B = UE.UKismetMathLibrary.RandomFloat()
		local BaseColor = {}
		BaseColor[0] = UE.FLinearColor(R, G, B, 1.0)
		local Projectile = self.World:SpawnActor(self.ProjectileClass, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self, self.Instigator, "Weapon.BP_DefaultProjectile_C", BaseColor)
	end
end

return BP_DefaultWeapon_C
