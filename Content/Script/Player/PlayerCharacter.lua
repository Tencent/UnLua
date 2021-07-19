require "UnLua"

local PlayerCharacter = Class("BP_CharacterBase_C")

local Lerp = UE4.UKismetMathLibrary.Lerp

--function BP_PlayerCharacter_C:UserConstructionScript()
--end

function PlayerCharacter:OnZoomInOutUpdate(Alpha)
	local FOV = Lerp(self.DefaultFOV, self.Weapon.AimingFOV, Alpha)
	self.Camera:SetFieldOfView(FOV)
end

function PlayerCharacter:SpawnWeapon()
	local World = self:GetWorld()
	if not World then
		return
	end
	local WeaponClass = UE4.UClass.Load("/Game/Core/Blueprints/Weapon/BP_DefaultWeapon.BP_DefaultWeapon")
	local NewWeapon = World:SpawnActor(WeaponClass, self:GetTransform(), UE4.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self, self, "Weapon.BP_DefaultWeapon_C")
	return NewWeapon
end

function PlayerCharacter:ReceiveBeginPlay()
	self.Super.ReceiveBeginPlay(self)
	self.DefaultFOV = self.Camera.FieldOfView
	self.TimerHandle = UE4.UKismetSystemLibrary.K2_SetTimerDelegate({self, BP_PlayerCharacter_C.FallCheck}, 1.0, true)
	local InterpFloats = self.ZoomInOut.TheTimeline.InterpFloats
	local FloatTrack = InterpFloats:GetRef(1)
	FloatTrack.InterpFunc:Bind(self, BP_PlayerCharacter_C.OnZoomInOutUpdate)
end

function PlayerCharacter:ReceiveDestroyed()
	UE4.UKismetSystemLibrary.K2_ClearTimerHandle(self, self.TimerHandle)
end

function PlayerCharacter:UpdateAiming(IsAiming)
	if self.Weapon then
		if IsAiming then
			self.ZoomInOut:Play()
		else
			self.ZoomInOut:Reverse()
		end
	end
end

function PlayerCharacter:FallCheck()



	local Location = self:K2_GetActorLocation()
	if Location.Z < -200.0 then
		UE4.UKismetSystemLibrary.ExecuteConsoleCommand(self, "RestartLevel")
	end
end

function PlayerCharacter:GetWeaponTraceInfo()
	local TraceLocation = self.Camera:K2_GetComponentLocation()
	local TraceDirection = self.Camera:GetForwardVector()
	return TraceLocation, TraceDirection
end

--[[
function BP_PlayerCharacter_C:GetWeaponTraceInfo(TraceStart, TraceDirection)
	self.Camera:K2_GetComponentLocation(TraceStart)
	self.Camera:GetForwardVector(TraceDirection)
end
]]

return PlayerCharacter
