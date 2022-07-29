require "UnLua"

local M = Class()

function M:TestForIssue445()
    return UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue445/UMG_Issue445.UMG_Issue445_C")
end

return M
