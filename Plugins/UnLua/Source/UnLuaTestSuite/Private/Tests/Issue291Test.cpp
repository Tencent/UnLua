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

        const auto World = UWorld::CreateWorld(EWorldType::Game, false, "UnLuaTest");
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);

        const auto ActorClass = LoadClass<AActor>(nullptr, TEXT("/Game/Tests/Regression/Issue291/BP_UnLuaTestActor_Issue291.BP_UnLuaTestActor_Issue291_C"));
        const auto Actor1 = World->SpawnActor(ActorClass);
        const auto Actor2 = World->SpawnActor(ActorClass);

        UnLua::PushUObject(L, Actor1);
        lua_setglobal(L, "G_Actor1");
        UnLua::PushUObject(L, Actor2);
        lua_setglobal(L, "G_Actor2");

        const char* Chunk = "\
            local function func() end\
            G_Actor1.Capsule.OnInputTouchBegin:Add(G_Actor2.Capsule, func)\
        ";

        UnLua::RunChunk(L, Chunk);

        const auto Components = Actor1->GetComponentsByClass(UCapsuleComponent::StaticClass());
        const auto CapsuleComponent = (UCapsuleComponent*)Components[0];

        RUNNER_TEST_TRUE(CapsuleComponent->OnInputTouchBegin.IsBound());

        lua_gc(L, LUA_GCCOLLECT, 0);
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);

        RUNNER_TEST_TRUE(CapsuleComponent->OnInputTouchBegin.IsBound());

        return true;
    }
};

IMPLEMENT_AI_LATENT_TEST(FUnLuaTest_Issue291, TEXT("UnLua.Regression.Issue291 delegate绑定本身不在lua里有引用的object会提前销毁"))

#endif //WITH_DEV_AUTOMATION_TESTS
