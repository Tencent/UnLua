require "UnLua"

local M = Class()

local function run(self)
    local action = NewObject(UE.UUnLuaLatentAction, self)
    action:SetTickableWhenPaused(true)
    local info = action:CreateInfoForLegacy()
    UE.UKismetSystemLibrary.Delay(self, 0.5, info)

    _G.Flag = true
end

function M:Construct()
    UE.UGameplayStatics.SetGamePaused(self, true)
    coroutine.resume(coroutine.create(run), self)
end

return M
