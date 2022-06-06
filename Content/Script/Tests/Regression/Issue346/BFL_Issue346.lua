require "UnLua"

local M = Class()

function M.Test()
    return M.Overridden.Test() .. " and lua"
end

return M
