local M = UnLua.Class()

function M:GetTestObject()
    return NewObject(UE.UObjectForIssue500)
end

return M