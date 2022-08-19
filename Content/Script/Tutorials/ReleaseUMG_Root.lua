---@type ReleaseUMG_Root_C
local M = UnLua.Class()

function M:Construct()
    print("Root Construct")
    self.Button_AddNew.OnClicked:Add(self, self.OnAddNew)
    self.Button_ReleaseAll.OnClicked:Add(self, self.OnReleaseAll)
end

function M:OnAddNew()
    print("Root Add New")
    local widget_class = UE.UClass.Load("/Game/Tutorials/11_ReleaseUMG/ReleaseUMG_ItemParent.ReleaseUMG_ItemParent_C")
    local widget = NewObject(widget_class, self)
    self.VerticalBox_Panel:AddChildToVerticalBox(widget)
end

function M:OnReleaseAll()
    self:RemoveFromViewport()
end

function M:Destruct()
    print("Root Destruct")
    self:Release()
end

return M
