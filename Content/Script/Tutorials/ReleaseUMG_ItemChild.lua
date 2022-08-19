---@type ReleaseUMG_ItemChild_C
local M = UnLua.Class()

function M:Construct()
    print("ItemChild Construct")
    self.Button_Remove.OnClicked:Add(self, self.OnRemove)
end

function M:Setup(parent)
    self.parent = parent
end

function M:OnRemove()
    print("ItemChild Remove")
    self.parent:Remove()
end

function M:Destruct()
    print("ItemChild Destruct")
    self:Release()
end

return M
