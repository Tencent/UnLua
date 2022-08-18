---@type UTPSGameInstance
local M = UnLua.Class()

function M:ReceiveInit()
	print("TPSGameInstance:ReceiveInit")
end

function M:ReceiveShutdown()
    print("TPSGameInstance:ReceiveShutdown")
end

return M