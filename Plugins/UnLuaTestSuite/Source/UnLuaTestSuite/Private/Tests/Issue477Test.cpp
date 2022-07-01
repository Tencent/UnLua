// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "Blueprint/UserWidget.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue477 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk = R"(
            local DataTable = UE.UObject.Load('/UnLuaTestSuite/Tests/Regression/Issue477/DataTable_Issue477.DataTable_Issue477')
            local RowStruct = UE.UObject.Load('/UnLuaTestSuite/Tests/Regression/Issue477/Struct_Issue477.Struct_Issue477')
            local RowNames = UE.UDataTableFunctionLibrary.GetDataTableRowNames(DataTable)
            local Sum = 0
            for i=1, RowNames:Num() do
                local RowName = RowNames[i]
                local Row = RowStruct()
                UE.UDataTableFunctionLibrary.GetDataTableRowFromName(DataTable, RowName, Row)
                Sum = Sum + Row.Number
                print(Row.ChildValue.StringValue)
            end
            return Sum
            )";
        UnLua::RunChunk(L, Chunk);
        const auto Result = (int)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(Result, 6);
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue477, TEXT("UnLua.Regression.Issue477 遍历蓝图结构体的DataTable会发生拷贝错误"))

#endif
