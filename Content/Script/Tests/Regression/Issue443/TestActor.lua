local M = UnLua.Class()

Result = 0

function M:LuaOnly()
    self:TestEmpty()
end

function M:CustomEvent()
    self:LuaOnly()
    self.Overridden.CustomEvent(self)
    print("Lua CustomEvent")
    self:TestEmpty()
    Result = Result + 1
end

return M
