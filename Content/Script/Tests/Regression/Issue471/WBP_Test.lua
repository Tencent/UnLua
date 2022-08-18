local M = UnLua.Class()

function M:Construct()
    Result = 1
    local delegate = { self, self.OnAnimFinished }
    self.Anim_Issue471:BindToAnimationFinished(self, delegate)
    self.Anim_Issue471:UnbindFromAnimationFinished(self, delegate)
end

function M:OnAnimFinished()
    Called = 0
end

return M
