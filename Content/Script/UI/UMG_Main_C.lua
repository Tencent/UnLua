---@type UMG_Main_C
local M = UnLua.Class()

function M:Construct()
	self.ExitButton.OnClicked:Add(self, M.OnClicked_ExitButton)	
	--self.ExitButton.OnClicked:Add(self, function(Widget) UE.UKismetSystemLibrary.ExecuteConsoleCommand(Widget, "exit") end )
end

function M:OnClicked_ExitButton()
	UE.UKismetSystemLibrary.ExecuteConsoleCommand(self, "exit")
end

return M
