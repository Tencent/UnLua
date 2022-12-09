local M = UnLua.Class()

function M:ReturnFull(Dest, Obj, Radius, bStop)
    Dest = UE.FVector(0, 0, 0)
    Obj = nil
    Radius = 2.0
    bStop = true
    return Dest, Obj, Radius, bStop
end

function M:ReturnPartial(Dest, Obj, Radius, bStop)
    return Dest
end

return M