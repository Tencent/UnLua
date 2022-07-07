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

struct FUnLuaTest_Issue375 : FUnLuaTestBase
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

        const auto Chunk = R"(
        local ParentClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue375/BP_ParentActor.BP_ParentActor_C")
        ParentActor = World:SpawnActor(ParentClass)
        
        local ChildClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue375/BP_ChildActor.BP_ChildActor_C")
        local ChildActor = World:SpawnActor(ChildClass)
        ChildActor:K2_DestroyActor()
        return ParentActor
        )";
        UnLua::RunChunk(L, Chunk);
        const auto ParentActor = (AActor*)UnLua::GetUObject(L, -1);

        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
        CollectGarbage(RF_NoFlags, true);

        const auto GetValue = ParentActor->FindFunction(TEXT("GetValue"));
        struct ParamsStruct
        {
            int32 ReturnValue;
        } Params;
        ParentActor->UObject::ProcessEvent(GetValue, &Params);
        RUNNER_TEST_EQUAL(Params.ReturnValue, 1);
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue375, TEXT("UnLua.Regression.Issue375 子类被销毁时，可能会令父类的Lua绑定失效"))

#endif //WITH_DEV_AUTOMATION_TESTS
