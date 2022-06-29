require "UnLua"

local M = Class()

function M.Test(v)
    local mt = getmetatable(M)
    print("mt=", mt)
    for k, v in pairs(mt) do
        print(k, v)
    end
    return M.Overridden.Test(v) .. " and " .. tostring(v)
end

return M
