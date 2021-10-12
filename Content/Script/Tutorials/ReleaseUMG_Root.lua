require "UnLua"

local M = Class()

function M:Construct()
    print("Root Construct")
    self.Button_AddNew.OnClicked:Add(self, self.OnAddNew)
    self.Button_ReleaseAll.OnClicked:Add(self, self.OnReleaseAll)
end

function M:OnAddNew()
    print("Root Add New")
end

function M:OnReleaseAll()
    self:RemoveFromViewport()
end

function M:Destruct()
    print("Root Destruct")
    self:Release()
end

return M
