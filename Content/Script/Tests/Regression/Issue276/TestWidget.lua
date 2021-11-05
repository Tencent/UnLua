require "UnLua"

local M = Class()

local function run(self)
    UE4.UKismetSystemLibrary.Delay(self, 0.5)
    _G.Flag = true
end

function M:Construct()
    UE4.UGameplayStatics.SetGamePaused(self, true)
    coroutine.resume(coroutine.create(run), self)
end

return M
