require "UnLua"

local BPI_Interfaces = UE.UClass.Load("/Game/Core/Blueprints/BPI_Interfaces.BPI_Interfaces_C")

local BP_PlayerController_C = Class()

--function BP_PlayerController_C:UserConstructionScript()
--end

function BP_PlayerController_C:ReceiveBeginPlay()
	if self:IsLocalPlayerController() then
		local Widget = UE.UWidgetBlueprintLibrary.Create(self, UE.UClass.Load("/Game/Core/UI/UMG_Main.UMG_Main_C"))
		Widget:AddToViewport()
	end

	self.ForwardVec = UE.FVector()
	self.RightVec = UE.FVector()
	self.ControlRot = UE.FRotator()

	self.BaseTurnRate = 45.0
	self.BaseLookUpRate = 45.0

	self.Overridden.ReceiveBeginPlay(self)
end

function BP_PlayerController_C:Turn(AxisValue)
	self:AddYawInput(AxisValue)
end

function BP_PlayerController_C:TurnRate(AxisValue)
	local DeltaSeconds = UE.UGameplayStatics.GetWorldDeltaSeconds(self)
	local Value = AxisValue * DeltaSeconds * self.BaseTurnRate
	self:AddYawInput(Value)
end

function BP_PlayerController_C:LookUp(AxisValue)
	self:AddPitchInput(AxisValue)
end

function BP_PlayerController_C:LookUpRate(AxisValue)
	local DeltaSeconds = UE.UGameplayStatics.GetWorldDeltaSeconds(self)
	local Value = AxisValue * DeltaSeconds * self.BaseLookUpRate
	self:AddPitchInput(Value)
end

function BP_PlayerController_C:MoveForward(AxisValue)
	if self.Pawn then
		local Rotation = self:GetControlRotation(self.ControlRot)
		Rotation:Set(0, Rotation.Yaw, 0)
		local Direction = Rotation:ToVector(self.ForwardVec)		-- Rotation:GetForwardVector()
		self.Pawn:AddMovementInput(Direction, AxisValue)
	end
end

function BP_PlayerController_C:MoveRight(AxisValue)
	if self.Pawn then
		local Rotation = self:GetControlRotation(self.ControlRot)
		Rotation:Set(0, Rotation.Yaw, 0)
		local Direction = Rotation:GetRightVector(self.RightVec)
		self.Pawn:AddMovementInput(Direction, AxisValue)
	end
end

function BP_PlayerController_C:Fire_Pressed()
	if self.Pawn then
		BPI_Interfaces.StartFire(self.Pawn)
	else
		UE.UKismetSystemLibrary.ExecuteConsoleCommand(self, "RestartLevel")
	end
end

function BP_PlayerController_C:Fire_Released()
	BPI_Interfaces.StopFire(self.Pawn)
end

function BP_PlayerController_C:Aim_Pressed()
	BPI_Interfaces.UpdateAiming(self.Pawn, true)
end

function BP_PlayerController_C:Aim_Released()
	BPI_Interfaces.UpdateAiming(self.Pawn, false)
end

return BP_PlayerController_C
