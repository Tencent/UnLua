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
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#if UNLUA_LEGACY_ARGS_PASSING

struct FUnLuaTest_Issue362 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk = R"(
        local Stub = NewObject(UE.UUnLuaTestStub)
        Stub.Issue362Delegate:Bind(Stub, function(_, InArray)
            InArray:Add(1)
        end)
        local Array = UE.TArray(0)
        Stub.Issue362Delegate:Execute(Array)
        return Array:Length()
        )";
        UnLua::RunChunk(L, Chunk);

        const auto Actual = (int)lua_tonumber(L, -1);
        RUNNER_TEST_EQUAL(Actual, 1);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue362, TEXT("UnLua.Regression.Issue362 TArray引用做委托参数在lua中修改不生效"))

#endif
#endif
