require "UnLua"

local ABP_AICharacter_C = Class()

function ABP_AICharacter_C:AnimNotify_NotifyPhysics()
	UE.UBPI_Interfaces_C.ChangeToRagdoll(self.Pawn)
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
	local Character = Pawn:Cast(UE.ABP_CharacterBase_C)
	if Character then
		if Character.IsDead and not self.IsDead then
			self.IsDead = true
			self.DeathAnimIndex = UE.UKismetMathLibrary.RandomIntegerInRange(0, 2)
		end
	end
end

return ABP_AICharacter_C
