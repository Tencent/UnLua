local M = UnLua.Class()

local function run(self)
    local action = NewObject(UE.UUnLuaLatentAction, self)
    action:SetTickableWhenPaused(true)
    local info = action:CreateInfo()
    UE.UKismetSystemLibrary.Delay(self, 0.5, info)
    _G.Flag = true
end

function M:Construct()
    UE.UGameplayStatics.SetGamePaused(self, true)
    coroutine.resume(coroutine.create(run), self)
end

return M
