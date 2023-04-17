local M = UnLua.Class()

function M:ReceiveBeginPlay()
    print("self=", self, self:GetName())
    print("self.Overridden=", self.Overridden)
    print("self.Overridden.ReceiveBeginPlay=", self.Overridden.ReceiveBeginPlay)
    print("self.ReceiveBeginPlay=", self.ReceiveBeginPlay)
    self.Overridden.ReceiveBeginPlay(self)
    print("Hello in Lua")
    self:IncreaseCounter()
end

function M:IncreaseCounter()
    _G.Counter = _G.Counter or 0
    _G.Counter = _G.Counter + 1
end

return M