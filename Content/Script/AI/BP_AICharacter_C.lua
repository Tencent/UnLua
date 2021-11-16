require "UnLua"

local BP_AICharacter_C= Class("BP_CharacterBase_C")

function BP_AICharacter_C:Initialize(Initializer)
	self.Super.Initialize(self)
	self.Damage = 128.0
	--self.DamageType = UE.UClass.Load("/Script/Engine.DamageType")
	self.DamageType = UE.UClass.Load("UDamageType")
end

--function BP_AICharacter_C:UserConstructionScript()
--end

function BP_AICharacter_C:ReceiveBeginPlay()
	self.Super.ReceiveBeginPlay(self)
	self.Sphere.OnComponentBeginOverlap:Add(self, BP_AICharacter_C.OnComponentBeginOverlap_Sphere)
end

function BP_AICharacter_C:Died(DamageType)
	self.Super.Died(self, DamageType)
	self.Sphere:SetCollisionEnabled(UE.ECollisionEnabled.NoCollision)
	local NewLocation = UE.FVector(0.0, 0.0, self.CapsuleComponent.CapsuleHalfHeight)
	local SweepHitResult = UE.FHitResult()
	self.Mesh:K2_SetRelativeLocation(NewLocation, false, SweepHitResult, false)
	self.Mesh:SetAllBodiesBelowSimulatePhysics(self.BoneName, true, true)
	local GameMode = UE.UGameplayStatics.GetGameMode(self)
	UE.UBPI_Interfaces_C.NotifyEnemyDied(GameMode)
	--self.Sphere.OnComponentBeginOverlap:Remove(self, BP_AICharacter_C.OnComponentBeginOverlap_Sphere)
end

function BP_AICharacter_C:OnComponentBeginOverlap_Sphere(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult)
	local PlayerCharacter = OtherActor:Cast(UE.ABP_PlayerCharacter_C)
	if PlayerCharacter then
		local Controller = self:GetController()
		UE.UGameplayStatics.ApplyDamage(PlayerCharacter, self.Damage, Controller, self, self.DamageType)
	end
end

return BP_AICharacter_C
