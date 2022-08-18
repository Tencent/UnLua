---@type ReleaseUMG_ItemParent_C
local M = UnLua.Class()

function M:Construct()
    print("ItemParent Construct", self:GetName())
    self.ItemChild:Setup(self)
end

function M:Remove()
    self:RemoveFromViewport()
end

function M:Destruct()
    print("ItemParent Destruct")
    self:Release()
end

return M
