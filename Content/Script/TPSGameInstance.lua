require "UnLua"

local M = Class()

function M:ReceiveInit()
	print("TPSGameInstance:ReceiveInit")
end

function M:ReceiveShutdown()
    print("TPSGameInstance:ReceiveShutdown")
end

return M