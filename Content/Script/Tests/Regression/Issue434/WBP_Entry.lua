require "UnLua"

local M = Class()

function M:InitState(Parent, Name)
    self.Name = Name
    local Handler = function(self, Count) M.OnTestDelegate(self, Count) end
    Parent.TestDelegate:Add(self, Handler)
end

function M:OnTestDelegate(Count)
    self.Text:SetText(tostring(Count))
    _G[self.Name] = Count
end

return M