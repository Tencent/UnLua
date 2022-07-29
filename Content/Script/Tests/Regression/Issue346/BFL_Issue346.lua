require "UnLua"

local M = Class()

function M.Test(v)
    return M.Overridden.Test(v) .. " and " .. tostring(v)
end

return M
