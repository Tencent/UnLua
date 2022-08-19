---@type BP_WeaponBase_C
local M = UnLua.Class("Weapon.BP_WeaponBase_C")

function M:UserConstructionScript()
	self.Super.UserConstructionScript(self)
	self.InfiniteAmmo = true
	self.ProjectileClass = UE.UClass.Load("/Game/Core/Blueprints/Weapon/BP_DefaultProjectile.BP_DefaultProjectile_C")
	self.MuzzleSocketName = "Muzzle"
	self.World = self:GetWorld()
end

function M:SpawnProjectile()
	local Transform = self:GetFireInfo()
	local R = UE.UKismetMathLibrary.RandomFloat()
	local G = UE.UKismetMathLibrary.RandomFloat()
	local B = UE.UKismetMathLibrary.RandomFloat()
	local BaseColor = {}
	BaseColor[0] = UE.FLinearColor(R, G, B, 1.0)
	self.World:SpawnActor(self.ProjectileClass, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self, self.Instigator, "Weapon.BP_DefaultProjectile_C", BaseColor)
end

return M
