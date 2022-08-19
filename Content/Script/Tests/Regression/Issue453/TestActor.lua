local M = UnLua.Class()

function M:ReceiveBeginPlay()
    local Struct1 = UE.UObject.Load("/UnLuaTestSuite/Tests/Regression/Issue453/Struct_Issue453")
    local Array1 = UE.TArray(Struct1)
    Array1:Add(Struct1())
    Array1:Add(Struct1())
    Result1_BP = self:Test1(Array1)
    Result1_Lua = Array1:Num()

    local Struct2 = UE.UObject.Load("/UnLuaTestSuite/Tests/Regression/Issue453/Struct_Issue453_OneField")
    local Array2 = UE.TArray(Struct2)
    Array2:Add(Struct2())
    Array2:Add(Struct2())
    Result2_BP = self:Test2(Array2)
    Result2_Lua = Array2:Num()
end

return M
