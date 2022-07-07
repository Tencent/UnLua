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

struct FUnLuaTest_Issue314 : FUnLuaTestBase
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
        lua_setglobal(L, "G_World");

        const auto Chunk = "\
        local ParentClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Regression/Issue314/BP_UnLuaTestActor_Issue314_Parent.BP_UnLuaTestActor_Issue314_Parent_C')\
        local ParentActor = G_World:SpawnActor(ParentClass)\
        local ChildClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Regression/Issue314/BP_UnLuaTestActor_Issue314_Child.BP_UnLuaTestActor_Issue314_Child_C')\
        local ChildActor = G_World:SpawnActor(ChildClass)\
        print('access ParentActor.Cube -> ', ParentActor.Cube)\
        Result = ChildActor.Cube ~= nil\
        \
        ";

        UnLua::RunChunk(L, Chunk);

        lua_getglobal(L, "Result");
        const auto Result = lua_toboolean(L, -1);
        RUNNER_TEST_TRUE(Result);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue314, TEXT("UnLua.Regression.Issue314 访问过父类不存在的字段，后续在子类中也只能取到nil"))

#endif //WITH_DEV_AUTOMATION_TESTS
