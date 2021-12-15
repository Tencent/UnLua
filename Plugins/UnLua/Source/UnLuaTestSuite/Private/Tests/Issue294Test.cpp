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

#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue294 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const char* Chunk1 = "\
            return UE.UUnLuaTestFunctionLibrary.TestForIssue294('A', 1)\
            ";
        UnLua::RunChunk(L, Chunk1);
        const auto Actual1 = (int32)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(Actual1, 0);

        const char* Chunk2 = "\
            local Callback = function(_, Value1, Value2)\
                G_Value1 = Value1\
                G_Value2 = Value2\
            end\
            return UE.UUnLuaTestFunctionLibrary.TestForIssue294('A', 1, { UE.UUnLuaTestFunctionLibrary, Callback }, UE.TArray(UE.FColor))\
            ";
        UnLua::RunChunk(L, Chunk2);
        const auto Actual2 = (int32)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(Actual2, 0);
        lua_pop(L, 1);

        lua_getglobal(L, "G_Value1");
        const auto G_Value1 = (int32)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(G_Value1, 1);
        lua_pop(L, 1);

        lua_getglobal(L, "G_Value2");
        const auto G_Value2 = (UClass*)UnLua::GetUObject(L, -1);
        RUNNER_TEST_EQUAL(G_Value2, UUnLuaTestFunctionLibrary::StaticClass());

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue294, TEXT("UnLua.Regression.Issue294 AutoCreateRefTerm标记的参数需要生成默认值"))

#endif //WITH_DEV_AUTOMATION_TESTS
