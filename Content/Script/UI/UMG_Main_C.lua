require "UnLua"

local UMG_Main_C = Class()

function UMG_Main_C:Construct()
	self.ExitButton.OnClicked:Add(self, UMG_Main_C.OnClicked_ExitButton)	
	--self.ExitButton.OnClicked:Add(self, function(Widget) UE.UKismetSystemLibrary.ExecuteConsoleCommand(Widget, "exit") end )
end

function UMG_Main_C:OnClicked_ExitButton()
	UE.UKismetSystemLibrary.ExecuteConsoleCommand(self, "exit")
end

return UMG_Main_C
