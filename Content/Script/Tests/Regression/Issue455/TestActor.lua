local M = UnLua.Class()

function M:Test(IntArray, StringArray)
    print("=======Test ", IntArray, StringArray)
    Result = ""
    for i=1, IntArray:Num() do
        print("IntArray:", i, IntArray[i])
        Result = Result .. tostring(IntArray[i])
    end
    for i=1, StringArray:Num() do
        print("StringArray:", i, StringArray[i])
        Result = Result .. StringArray[i]
    end
end


return M
