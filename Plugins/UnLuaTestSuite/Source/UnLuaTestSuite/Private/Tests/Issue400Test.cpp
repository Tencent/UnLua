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
#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue400 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk = R"(
            local Stub1 = NewObject(UE.UUnLuaTestStub)
            local Stub2 = NewObject(UE.UUnLuaTestStub)
            local Stub3 = NewObject(UE.UUnLuaTestStub)

            local Counter = 0
            local function Callback()
                Counter = Counter + 1
            end

            Stub1.SimpleEvent:Add(Stub3, Callback)
            Stub2.SimpleEvent:Add(Stub3, Callback)

            Stub1.SimpleEvent:Broadcast()
            Stub2.SimpleEvent:Broadcast()

            Result = Counter
        )";
        UnLua::RunChunk(L, Chunk);

        lua_getglobal(L, "Result");
        const auto Result = static_cast<int32>(lua_tonumber(L, -1));
        RUNNER_TEST_EQUAL(Result, 2);

        CollectGarbage(RF_NoFlags);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue400, TEXT("UnLua.Regression.Issue400 不同实例的多播委托绑定相同实例函数，清理时崩溃"))

#endif //WITH_DEV_AUTOMATION_TESTS
