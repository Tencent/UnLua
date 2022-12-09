---@type EUB_RunMe_C
local M = UnLua.Class()

function M:Construct()
    local WidgetClass = UE.UClass.Load("/Game/Tutorials/14_EditorExtension/EditorExtension_UI.EditorExtension_UI_C")
    local Widget = UE.UWidgetBlueprintLibrary.Create(self, WidgetClass)
    self.RootPanel:AddChildToCanvas(Widget)
    local Slot = UE.UWidgetLayoutLibrary.SlotAsCanvasSlot(Widget)
    Slot:SetPosition(UE.FVector2D(540, 400))
end

return M
