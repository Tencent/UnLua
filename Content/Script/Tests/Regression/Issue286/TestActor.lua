require "UnLua"

local M = Class()

function M:ReceiveBeginPlay()
    local map = UE4.TMap(UE4.FName, UE4.FVector)
	local bpmap = self.MyMap
	local v = UE4.FVector(1, 2, 3)
	map:Add("a", v)
	bpmap:Add("a", v)
	local expected = map:FindRef("a")
	local actual = bpmap:FindRef("a")
    if expected == actual then
        Result = "passed"
    else
        Result = string.format("expected %s but got %s", expected, actual)
    end
end

return M
