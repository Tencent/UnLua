local M = UnLua.Class()

function M:Test1(Array)
    return Array:Length()
end

function M:Test2()
    local Array = UE.TArray(0)
    Array:Add(1)
    Array:Add(2)
    return Array
end

function M:SetResult(Key, Value)
    _G[Key] = Value
end

return M
