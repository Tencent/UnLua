local M = UnLua.Class()

function M:ReceiveBeginPlay()
    Result1 = 10
    Result2 = self:GetFromBlueprint()
end

function M:TestForIssue300()
    return 10
end

return M
