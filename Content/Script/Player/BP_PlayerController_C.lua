---@type BP_PlayerController_C
local M = UnLua.Class()

function M:UserConstructionScript()
	self.ForwardVec = UE.FVector()
	self.RightVec = UE.FVector()
	self.ControlRot = UE.FRotator()

	self.BaseTurnRate = 45.0
	self.BaseLookUpRate = 45.0	
end

function M:ReceiveBeginPlay()
	if self:IsLocalPlayerController() then
		local Widget = UE.UWidgetBlueprintLibrary.Create(self, UE.UClass.Load("/Game/Core/UI/UMG_Main.UMG_Main_C"))
		Widget:AddToViewport()
	end

	self.Overridden.ReceiveBeginPlay(self)
end

function M:Turn(AxisValue)
	self:AddYawInput(AxisValue)
end

function M:TurnRate(AxisValue)
	local DeltaSeconds = UE.UGameplayStatics.GetWorldDeltaSeconds(self)
	local Value = AxisValue * DeltaSeconds * self.BaseTurnRate
	self:AddYawInput(Value)
end

function M:LookUp(AxisValue)
	self:AddPitchInput(AxisValue)
end

function M:LookUpRate(AxisValue)
	local DeltaSeconds = UE.UGameplayStatics.GetWorldDeltaSeconds(self)
	local Value = AxisValue * DeltaSeconds * self.BaseLookUpRate
	self:AddPitchInput(Value)
end

function M:MoveForward(AxisValue)
	if self.Pawn then
		local Rotation = self:GetControlRotation(self.ControlRot)
		Rotation:Set(0, Rotation.Yaw, 0)
		local Direction = Rotation:ToVector(self.ForwardVec)		-- Rotation:GetForwardVector()
		self.Pawn:AddMovementInput(Direction, AxisValue)
	end
end

function M:MoveRight(AxisValue)
	if self.Pawn then
		local Rotation = self:GetControlRotation(self.ControlRot)
		Rotation:Set(0, Rotation.Yaw, 0)
		local Direction = Rotation:GetRightVector(self.RightVec)
		self.Pawn:AddMovementInput(Direction, AxisValue)
	end
end

function M:Fire_Pressed()
	if self.Pawn then
		self.Pawn:StartFire_Server()
	else
		UE.UKismetSystemLibrary.ExecuteConsoleCommand(self, "RestartLevel")
	end
end

function M:Fire_Released()
	if self.Pawn then
		self.Pawn:StopFire_Server()
	end
end

function M:Aim_Pressed()
	if self.Pawn then
		local BPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")
		BPI_Interfaces.UpdateAiming(self.Pawn, true)
	end
end

function M:Aim_Released()
	if self.Pawn then
		local BPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")
		BPI_Interfaces.UpdateAiming(self.Pawn, false)
	end
end

return M
