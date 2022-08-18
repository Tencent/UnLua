local M = UnLua.Class()

function M:InitState(Parent, Name)
    self.Name = Name
    Parent.TestDelegate:Add(self, M.OnTestDelegate)
end

function M:OnTestDelegate(Count)
    self.Text:SetText(tostring(Count))
    _G[self.Name] = Count
end

return M