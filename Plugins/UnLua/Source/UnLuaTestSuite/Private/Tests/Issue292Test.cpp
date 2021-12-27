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

struct FUnLuaTest_Issue292 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto World = GetWorld();
        AActor* Actor = World->SpawnActor(AActor::StaticClass());
        UnLua::PushUObject(L, Actor, false);
        lua_setglobal(L, "G_Actor");
        Actor->K2_DestroyActor();

        const char* Chunk = "\
            return G_Actor:IsValid(), UE.UKismetSystemLibrary.IsValid(G_Actor)\
            ";
        UnLua::RunChunk(L, Chunk);

        RUNNER_TEST_FALSE(lua_toboolean(L, -1));
        RUNNER_TEST_FALSE(lua_toboolean(L, -2));

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue292, TEXT("UnLua.Regression.Issue292 actor调用K2_DestroyActor后，立刻调用IsValid会返回true"))

#endif //WITH_DEV_AUTOMATION_TESTS
