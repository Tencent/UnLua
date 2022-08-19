local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local map = UE.TMap(UE.FName, UE.FVector)
	local bpmap = self.MyMap
	local v = UE.FVector(1, 2, 3)
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
