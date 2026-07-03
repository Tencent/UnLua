local M = UnLua.Class()

function M:CollectInfo()
    return self.Overridden.CollectInfo(self) + 1
end

return M
