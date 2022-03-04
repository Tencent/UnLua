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

struct FUnLuaTest_Issue372 : FUnLuaTestBase
{
    virtual FString GetMapName() override { return TEXT("/Game/Tests/Regression/Issue372/Issue372"); }

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

        UnLua::PushUObject(L, World);
        const auto NextRef = luaL_ref(L, LUA_REGISTRYINDEX);
        luaL_unref(L, LUA_REGISTRYINDEX, NextRef);

        const auto Chunk = R"(
        local ActorClass = UE.UClass.Load("/Game/Tests/Binding/BP_UnLuaTestActor_StaticBinding.BP_UnLuaTestActor_StaticBinding_C")
        Actor = World:SpawnActor(ActorClass)
        )";
        UnLua::RunChunk(L, Chunk);

        lua_rawgeti(L, LUA_REGISTRYINDEX, NextRef);
        auto Actual = lua_type(L, -1);
        RUNNER_TEST_EQUAL(Actual, LUA_TTABLE);

        UnLua::RunChunk(L, "Actor:K2_DestroyActor()");
        lua_rawgeti(L, LUA_REGISTRYINDEX, NextRef);
        Actual = lua_type(L, -1);
        RUNNER_TEST_EQUAL(Actual, LUA_TNIL);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue372, TEXT("UnLua.Regression.Issue372 Actor调用K2_DestroyActor销毁时，lua部分会在注册表残留"))

#endif //WITH_DEV_AUTOMATION_TESTS
