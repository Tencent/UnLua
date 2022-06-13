require "UnLua"

local M = Class()

function M:Test(IntArray,StringArray)
    print("=======Test ", IntArray,StringArray)
    for i=1, IntArray:Num() do
        print("IntArray:", i, IntArray[i])
    end
    for i=1, StringArray:Num() do
        print("StringArray:", i, StringArray[i])
    end
end


return M
