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

#include "Tests/Issue505Test.h"
#include "Blueprint/UserWidget.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue505 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk = R"(
            local Array = UE.UUnLuaIssue505Helper.GetStruct().Array
            collectgarbage("collect")
            collectgarbage("collect")
            UE.UKismetSystemLibrary.CollectGarbage()
            collectgarbage("collect")
            collectgarbage("collect")
            return Array:Length(), Array:Get(1).Title, Array:Get(2).Title
        )";
        UnLua::RunChunk(L, Chunk);

        const auto A = (int32)lua_tointeger(L, -3);
        const auto B = lua_tostring(L, -2);
        const auto C = lua_tostring(L, -1);

        RUNNER_TEST_EQUAL(A, 2);
        RUNNER_TEST_EQUAL(B, "Row1");
        RUNNER_TEST_EQUAL(C, "Row2");
        
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue505, TEXT("UnLua.Regression.Issue505 Lua持有结构体下的TArray字段，在结构体本身被GC后访问该数组会导致崩溃"))

#endif
