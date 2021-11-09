require "UnLua"

local M = Class()

local function run(self)
    local action = NewObject(UE4.UUnLuaLatentAction, self)
    action:SetTickableWhenPaused(true)
    local info = action:CreateInfoForLegacy()
    UE4.UKismetSystemLibrary.Delay(self, 0.5, info)

    _G.Flag = true
end

function M:Construct()
    UE4.UGameplayStatics.SetGamePaused(self, true)
    coroutine.resume(coroutine.create(run), self)
end

return M
