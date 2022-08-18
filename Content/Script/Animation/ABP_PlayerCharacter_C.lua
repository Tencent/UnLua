---@type ABP_PlayerCharacter_C
local M = UnLua.Class()

function M:AnimNotify_NotifyPhysics()
	local BPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")
	BPI_Interfaces.ChangeToRagdoll(self.Pawn)
end

function M:BlueprintBeginPlay()
	self.Velocity = UE.FVector()
	self.ForwardVec = UE.FVector()
	self.RightVec = UE.FVector()
	self.ControlRot = UE.FRotator()
	self.Pawn = self:TryGetPawnOwner()
end

function M:BlueprintUpdateAnimation(DeltaTimeX)
	local Pawn = self:TryGetPawnOwner(self.Pawn)
	if not Pawn then
		return
	end
	local Vel = Pawn:GetVelocity(self.Velocity)
	if not Vel then
		return
	end
	local BP_CharacterBase = UE.UClass.Load("/Game/Core/Blueprints/BP_CharacterBase.BP_CharacterBase_C")
	local Character = Pawn:Cast(BP_CharacterBase)
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

return M
