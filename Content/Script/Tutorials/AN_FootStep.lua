local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

function M:Received_Notify(MeshComp, Animation)
    Screen.Print("FootStep")
end

return M