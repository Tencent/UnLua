require "UnLua"

local M = Class()

function M:Test1(Array1)
    print("=======Test1 Start Array1=", Array1)
    for i=1, Array1:Num() do
        print("Test1:", i, Array1[i])
    end
end

function M:Test2(Array2)
    print("=======Test2 Start Array2=", Array2)
    for i=1, Array2:Num() do
        print("Test2:", i, Array2[i])
    end
end

return M
