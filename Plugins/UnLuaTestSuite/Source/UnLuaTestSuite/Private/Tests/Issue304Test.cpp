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

struct FUnLuaTest_Issue304 : FUnLuaTestBase
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
        Stub.Issue304Event:Add(Stub, function(_, InArray)
            Result = InArray:Get(2)
        end)
        
        local Array = UE.TArray('')
        Array:Add('Hello')
        Array:Add('World')
        Stub.Issue304Event:Broadcast(Array)
        return Result
        )";

        UnLua::RunChunk(L, Chunk);

        const auto Result = lua_tostring(L, -1);
        RUNNER_TEST_EQUAL(Result, "World");

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue304, TEXT("UnLua.Regression.Issue304 Lua中监听delegate，接收TArray参数，使用Get获取其中元素导致崩溃"))

#endif //WITH_DEV_AUTOMATION_TESTS
