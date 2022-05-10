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

struct FUnLuaTest_Issue291 : FUnLuaTestBase
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

        const char* Chunk = "\
            local Flag = false\
            local function test(x, y, z)\
                Flag = true\
            end\
            local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Regression/Issue291/BP_UnLuaTestActor_Issue291.BP_UnLuaTestActor_Issue291_C')\
            local Actor = G_World:SpawnActor(ActorClass)\
            Actor.Capsule.OnInputTouchBegin:Add(Actor.Capsule, test)\
            collectgarbage('collect')\
            \
            Actor.Capsule.OnInputTouchBegin:Broadcast(0)\
            Actor.Capsule.OnInputTouchBegin:Remove(Actor.Capsule, test)\
            return Flag\
        ";

        UnLua::RunChunk(L, Chunk);

        const auto Result = !!lua_toboolean(L, -1);
        RUNNER_TEST_TRUE(Result);
        
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue291, TEXT("UnLua.Regression.Issue291 delegate绑定本身不在lua里有引用的object，在绑定后触发gc会导致无法调用绑定的函数"))

#endif //WITH_DEV_AUTOMATION_TESTS
