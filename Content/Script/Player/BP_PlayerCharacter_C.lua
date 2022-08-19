---@type BP_PlayerCharacter_C
local M= UnLua.Class("BP_CharacterBase_C")

local Lerp = UE.UKismetMathLibrary.Lerp

--function M:UserConstructionScript()
--end

function M:OnZoomInOutUpdate(Alpha)
	local FOV = Lerp(self.DefaultFOV, self.Weapon.AimingFOV, Alpha)
	self.Camera:SetFieldOfView(FOV)
end

function M:SpawnWeapon()
	local World = self:GetWorld()
	if not World then
		return
	end
	local WeaponClass = UE.UClass.Load("/Game/Core/Blueprints/Weapon/BP_DefaultWeapon.BP_DefaultWeapon_C")
	-- local NewWeapon = World:SpawnActor(WeaponClass, self:GetTransform(), UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, self, self, "Weapon.BP_DefaultWeapon_C")
	local sp = UE.FActorSpawnParameters()
	sp.SpawnCollisionHandlingOverride = UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn
	sp.Owner = self
	sp.Instigator = self

	local NewWeapon = World:SpawnActorEx(
		WeaponClass, self:GetTransform(), nil, "Weapon.BP_DefaultWeapon_C", sp)
	return NewWeapon
end

function M:ReceiveBeginPlay()
	self.Super.ReceiveBeginPlay(self)
	self.DefaultFOV = self.Camera.FieldOfView
	self.TimerHandle = UE.UKismetSystemLibrary.K2_SetTimerDelegate({self, M.FallCheck}, 1.0, true)
	local InterpFloats = self.ZoomInOut.TheTimeline.InterpFloats
	local FloatTrack = InterpFloats:GetRef(1)
	FloatTrack.InterpFunc:Bind(self, M.OnZoomInOutUpdate)
end

function M:ReceiveDestroyed()
	UE.UKismetSystemLibrary.K2_ClearTimerHandle(self, self.TimerHandle)
end

function M:UpdateAiming(IsAiming)
	if self.Weapon then
		if IsAiming then
			self.ZoomInOut:Play()
		else
			self.ZoomInOut:Reverse()
		end
	end
end

function M:FallCheck()
	local Location = self:K2_GetActorLocation()
	if Location.Z < -200.0 then
	    UE.UKismetSystemLibrary.ExecuteConsoleCommand(self, "RestartLevel")
	end
end

function M:GetWeaponTraceInfo()
	local TraceLocation = self.Camera:K2_GetComponentLocation()
	local TraceDirection = self.Camera:GetForwardVector()
	return TraceLocation, TraceDirection
end

--[[
function M:GetWeaponTraceInfo(TraceStart, TraceDirection)
	self.Camera:K2_GetComponentLocation(TraceStart)
	self.Camera:GetForwardVector(TraceDirection)
end
]]

return M
