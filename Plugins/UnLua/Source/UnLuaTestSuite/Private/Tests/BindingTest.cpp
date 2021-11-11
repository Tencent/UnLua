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
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

static void Run(TFunction<void(lua_State*, UWorld*)> Test)
{
    // SetUp
    UnLua::Startup();
    const auto L = UnLua::GetState();

    const auto World = UWorld::CreateWorld(EWorldType::Game, false, "UnLuaTest");
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(World);

    const FURL URL;
    World->InitializeActorsForPlay(URL);
    World->BeginPlay();
    World->bBegunPlay = true;

    UnLua::PushUObject(L, World, false);
    lua_setglobal(L, "World");

    // Perform Test
    Test(L, World);

    // TearDown
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    UnLua::Shutdown();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_StaticBinding, TEXT("UnLua.API.Binding.Static 静态绑定，继承UnLuaInterface::GetModuleName指定脚本路径"),
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_StaticBinding::RunTest(const FString& Parameters)
{
    Run([this](lua_State* L, UWorld* World)
    {
        const char* Chunk1 = "\
                    local ActorClass = UE4.UClass.Load('/Game/Tests/Binding/BP_UnLuaTestActor_StaticBinding.BP_UnLuaTestActor_StaticBinding_C')\
                    G_Actor = World:SpawnActor(ActorClass)\
                ";
        UnLua::RunChunk(L, Chunk1);


        World->Tick(LEVELTICK_All, 0.1f);

        const char* Chunk2 = "\
                return G_Actor:RunTest()\
                ";
        UnLua::RunChunk(L, Chunk2);

        const auto Error = lua_tostring(L, -1);
        TEST_EQUAL(Error, "");
    });

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_DynamicBinding, TEXT("UnLua.API.Binding.Dynamic 动态绑定，SpawnActor时指定脚本路径"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_DynamicBinding::RunTest(const FString& Parameters)
{
    Run([this](lua_State* L, UWorld* World)
    {
        const char* Chunk1 = "\
                    local ActorClass = UE4.UClass.Load('/Game/Tests/Binding/BP_UnLuaTestActor_DynamicBinding.BP_UnLuaTestActor_DynamicBinding_C')\
                    local Transform = UE4.FTransform()\
                    G_Actor = World:SpawnActor(ActorClass, Transform, UE4.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, nil, nil, 'Tests.Binding.BP_UnLuaTestActor')\
                ";
        UnLua::RunChunk(L, Chunk1);


        World->Tick(LEVELTICK_All, 0.1f);

        const char* Chunk2 = "\
                return G_Actor:RunTest()\
                ";
        UnLua::RunChunk(L, Chunk2);

        const auto Error = lua_tostring(L, -1);
        TEST_EQUAL(Error, "");
    });
    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
