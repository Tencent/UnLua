require "UnLua"

local M = Class()

local Screen = require "Tutorials.Screen"

function M:UserConstructionScript()
    self.Name = ""
    self.NameTextRender:SetText("")
end

function M:ReceivePossessed()
    self.Name = string.format("PLAYER_%d", self:GetController().PlayerState.PlayerId)
    self:Say("我来了")
end

function M:Say_RPC(text)
    local msg = string.format("[%s]说：%s", self.Name, text)
    Screen.Print(msg)
end

function M:OnRep_Name()
    self.NameTextRender:SetText(self.Name)
end

function M:SpaceBar_Pressed()
    self:Jump()
    self:Say("我跳~")
end

function M:MoveForward(AxisValue)
	self:AddMovementInput(UE.FVector(AxisValue, 0, 0), 100, false)
end

function M:MoveRight(AxisValue)
    self:AddMovementInput(UE.FVector(0, AxisValue, 0), 100, false)
end

return M
