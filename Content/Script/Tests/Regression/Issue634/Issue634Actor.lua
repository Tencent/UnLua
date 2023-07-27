local M =  UnLua.Class()

function M:Test(OutParam)
    print("Issue 634 Test", OutParam)
    return
end

return M