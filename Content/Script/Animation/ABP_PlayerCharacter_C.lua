require "UnLua"

local ABP_PlayerCharacter_C = Class()

function ABP_PlayerCharacter_C:AnimNotify_NotifyPhysics()
	UE.UBPI_Interfaces_C.ChangeToRagdoll(self.Pawn)
end

function ABP_PlayerCharacter_C:BlueprintBeginPlay()
	self.Velocity = UE.FVector()
	self.ForwardVec = UE.FVector()
	self.RightVec = UE.FVector()
	self.ControlRot = UE.FRotator()
	self.Pawn = self:TryGetPawnOwner()
end

function ABP_PlayerCharacter_C:BlueprintUpdateAnimation(DeltaTimeX)
	local Pawn = self:TryGetPawnOwner(self.Pawn)
	if not Pawn then
		return
	end
	local Vel = Pawn:GetVelocity(self.Velocity)
	if not Vel then
		return
	end
	local Character = Pawn:Cast(UE.ABP_CharacterBase_C)
	if Character then
		if Character.IsDead and not self.IsDead then
			self.IsDead = true
			self.DeathAnimIndex = UE.UKismetMathLibrary.RandomIntegerInRange(0, 2)
		end
	end
	local Speed = Vel:Size()
	self.Speed = Speed
	if Speed > 0.0 then
		Vel:Normalize()
		local Rot = Pawn:GetControlRotation(self.ControlRot)
		Rot:Set(0, Rot.Yaw, 0)
		local ForwardVec = Rot:GetForwardVector(self.ForwardVec)
		local RightVec = Rot:GetRightVector(self.RightVec)
		local DP0 = Vel:Dot(RightVec)
		local DP1 = Vel:Dot(ForwardVec)
		local Angle = UE.UKismetMathLibrary.Acos(DP1)
		if DP0 > 0.0 then
			self.Direction = Angle
		else
			self.Direction = -Angle
		end
	end
end

return ABP_PlayerCharacter_C
