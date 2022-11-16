local M = UnLua.Class()

function M:PlayerViewChanged(AddHexHandles, RemoveHexHandlers)
    print("==========PlayerViewChanged", AddHexHandles, RemoveHexHandlers)
    for k, v in pairs(RemoveHexHandlers) do
        print(k, v, v.Value)
    end
    print("==========PlayerViewChanged", RemoveHexHandlers:Num())
end

return M