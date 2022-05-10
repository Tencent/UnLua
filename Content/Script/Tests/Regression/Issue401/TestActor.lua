require "UnLua"

local M = Class()

function M:Test(Array)
    Result = Array:Length()
    print("Result=", Result)
    return Result
end

return M
