local M = UnLua.Class()

function M:Test(V)
    self.V = V:Copy()
end

return M
