local M = UnLua.Class()

function M:Test(StringArray, FloatArray, StructArray)
    Result1 = StringArray:Num()
    Result2 = FloatArray:Num()
    Result3 = StructArray:Num()
    print("Test From Lua", Result1, Result2, Result3)
end

return M