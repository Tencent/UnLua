require "UnLua"

local BPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")
local BP_CharacterBase = UE.UClass.Load("/Game/Core/Blueprints/BP_CharacterBase.BP_CharacterBase_C")

local ABP_AICharacter_C = Class()

function ABP_AICharacter_C:AnimNotify_NotifyPhysics()
	BPI_Interfaces.ChangeToRagdoll(self.Pawn)
end

function ABP_AICharacter_C:BlueprintBeginPlay()
	self.Velocity = UE.FVector()
	self.Pawn = self:TryGetPawnOwner()
end

function ABP_AICharacter_C:BlueprintUpdateAnimation(DeltaTimeX)
	local Pawn = self:TryGetPawnOwner(self.Pawn)
	if not Pawn then
		return
	end
	local Vel = Pawn:GetVelocity(self.Velocity)
	if not Vel then
		return
	end
	self.Speed = Vel:Size()
	local Character = Pawn:Cast(BP_CharacterBase)
	if Character then
		if Character.IsDead and not self.IsDead then
			self.IsDead = true
			self.DeathAnimIndex = UE.UKismetMathLibrary.RandomIntegerInRange(0, 2)
		end
	end
end

return ABP_AICharacter_C
