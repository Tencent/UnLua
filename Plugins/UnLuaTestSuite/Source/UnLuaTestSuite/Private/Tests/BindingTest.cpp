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

#include "Engine.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

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
                    local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_StaticBinding.BP_UnLuaTestActor_StaticBinding_C')\
                    G_Actor = World:SpawnActor(ActorClass)\
                ";
        UnLua::RunChunk(L, Chunk1);


        World->Tick(LEVELTICK_All, SMALL_NUMBER);

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
                    local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_DynamicBinding.BP_UnLuaTestActor_DynamicBinding_C')\
                    local Transform = UE.FTransform()\
                    G_Actor = World:SpawnActor(ActorClass, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, nil, nil, 'Tests.Binding.BP_UnLuaTestActor')\
                ";
        UnLua::RunChunk(L, Chunk1);


        World->Tick(LEVELTICK_All, SMALL_NUMBER);

        const char* Chunk2 = "\
                return G_Actor:RunTest()\
                ";
        UnLua::RunChunk(L, Chunk2);

        const auto Error = lua_tostring(L, -1);
        TEST_EQUAL(Error, "");
    });
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_ConflictedBinding, TEXT("UnLua.API.Binding.Priority 绑定冲突：静态绑定优先于动态绑定"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_ConflictedBinding::RunTest(const FString& Parameters)
{
    Run([this](lua_State* L, UWorld* World)
    {
        AddExpectedError(TEXT("conflicts"), EAutomationExpectedErrorFlags::Contains);

        const char* Chunk1 = "\
                    local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_StaticBinding.BP_UnLuaTestActor_StaticBinding_C')\
                    local Transform = UE.FTransform()\
                    G_Actor = World:SpawnActor(ActorClass, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, nil, nil, 'Tests.Binding.Foo')\
                ";
        UnLua::RunChunk(L, Chunk1);

        World->Tick(LEVELTICK_All, SMALL_NUMBER);

        const char* Chunk2 = "\
                return G_Actor:RunTest()\
                ";
        UnLua::RunChunk(L, Chunk2);

        const auto Error = lua_tostring(L, -1);
        TEST_EQUAL(Error, "");
    });
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_MultipleBinding, TEXT("UnLua.API.Binding.Multiple 多重绑定：同一个Lua脚本绑定到不同类"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_MultipleBinding::RunTest(const FString& Parameters)
{
    Run([this](lua_State* L, UWorld* World)
    {
        const char* Chunk1 = "\
                    local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_StaticBindingChild.BP_UnLuaTestActor_StaticBindingChild_C')\
                    G_Actor = World:SpawnActor(ActorClass)\
                ";
        UnLua::RunChunk(L, Chunk1);

        World->Tick(LEVELTICK_All, SMALL_NUMBER);

        const char* Chunk2 = "\
                return G_Actor:RunTest()\
                ";
        UnLua::RunChunk(L, Chunk2);

        const auto Error = lua_tostring(L, -1);
        TEST_EQUAL(Error, "");
    });

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_Overridden, TEXT("UnLua.API.Binding.Overridden 覆写：同一个Lua脚本绑定到不同类"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_Overridden::RunTest(const FString& Parameters)
{
    Run([this](lua_State* L, UWorld* World)
    {
        const char* Chunk = R"(
                    local ActorClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_StaticBindingChild.BP_UnLuaTestActor_StaticBindingChild_C')
                    G_Actor = World:SpawnActor(ActorClass)
                    return G_Actor:Greeting('ABC')
                )";
        UnLua::RunChunk(L, Chunk);

        World->Tick(LEVELTICK_All, SMALL_NUMBER);

        const auto Actual = lua_tostring(L, -1);
        TEST_EQUAL(Actual, "BP ABC Lua");
    });

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
