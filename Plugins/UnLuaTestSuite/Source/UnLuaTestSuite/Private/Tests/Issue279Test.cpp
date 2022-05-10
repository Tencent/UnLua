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

struct FUnLuaTest_Issue279 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto World = GetWorld();
        UnLua::PushUObject(L, World);
        lua_setglobal(L, "World");

        const auto Chunk1 = R"(
            local ActorClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue279/BP_TestActor.BP_TestActor_C")
            Actor = World:SpawnActor(ActorClass)
            Actor:Run()
        )";
        UnLua::RunChunk(L, Chunk1);

        const auto Chunk2 = R"(
            return Actor.V == UE.FVector(1, 2, 3)
        )";
        UnLua::RunChunk(L, Chunk2);

        const auto Result = (bool)lua_toboolean(L, -1);
        RUNNER_TEST_TRUE(Result);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue279, TEXT("UnLua.Regression.Issue279 lua覆写蓝图的同名函数之后，无法将特定类型的函数参数保存到lua对象中"))

#endif //WITH_DEV_AUTOMATION_TESTS
