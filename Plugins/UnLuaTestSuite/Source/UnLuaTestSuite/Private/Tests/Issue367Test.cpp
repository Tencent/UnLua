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

struct FUnLuaTest_Issue367 : FUnLuaTestBase
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
                local ActorClass = UE.UObject.Load('/UnLuaTestSuite/Tests/Misc/BP_UnLuaTestStubActor.BP_UnLuaTestStubActor_C')
                _G.MT = getmetatable(ActorClass) 
                local Actor = World:SpawnActor(ActorClass)
                Actor:K2_DestroyActor()
            )";
        UnLua::RunChunk(L, Chunk1);
        lua_gc(L, LUA_GCCOLLECT, 0); // Object_Delete -> DeleteUObjectRefs(ActorClass) 释放了C++侧的引用，导致ActorClass可以被UEGC
        CollectGarbage(RF_NoFlags, true); // UEGC了ActorClass，此时lua里ActorClass的元表还没有被luagc，但元表里对应的FClassDesc已经在FReflectionRegistry::NotifyUObjectDeleted中析构了

        const auto Chunk2 = R"(
                _G.ActorClass = UE.UObject.Load('/UnLuaTestSuite/Tests/Misc/BP_UnLuaTestStubActor.BP_UnLuaTestStubActor_C')
                _G.Actor = World:SpawnActor(ActorClass)
                _G.MT = nil
            )";
        UnLua::RunChunk(L, Chunk2);
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCCOLLECT, 0); // 元表被luagc -> Object_Delete -> UnRegisterClass导致FClassDesc再次被析构

        const auto Chunk3 = R"(
                Result = Actor.K2_DetachFromActor ~= nil
            )";
        UnLua::RunChunk(L, Chunk3);
        lua_getglobal(L, "Result");
        RUNNER_TEST_TRUE(lua_toboolean(L, -1));

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue367, TEXT("UnLua.Regression.Issue367 类型元表被LuaGC时，可能意外导致正在使用的FClassDesc被错误delete"))

#endif //WITH_DEV_AUTOMATION_TESTS
