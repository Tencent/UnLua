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

#include "UnLuaBase.h"
#include "UnLuaTemplate.h"
#include "Misc/AutomationTest.h"
#include "Engine.h"
#include "UnLuaTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibObjectSpec, "UnLua.API.UObject", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
    UWorld* World;
END_DEFINE_SPEC(FUnLuaLibObjectSpec)

void FUnLuaLibObjectSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();

        World = UWorld::CreateWorld(EWorldType::Game, false, "UnLuaTest");
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);

        const FURL URL;
        World->InitializeActorsForPlay(URL);
        World->BeginPlay();

        UnLua::PushUObject(L, World);
        lua_setglobal(L, "World");
    });

    Describe(TEXT("Load"), [this]()
    {
        It(TEXT("加载一个对象"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UClass* Expected = LoadObject<UClass>(nullptr, TEXT("/UnLuaTestSuite/Tests/Misc/BP_UnLuaTestStubActor.BP_UnLuaTestStubActor_C"));
            const char* Chunk = R"(
            local ActorClass = UE.UObject.Load('/UnLuaTestSuite/Tests/Misc/BP_UnLuaTestStubActor.BP_UnLuaTestStubActor_C')
            return ActorClass
            )";
            UnLua::RunChunk(L, Chunk);
            UClass* Actual = (UClass*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("IsValid"), [this]()
    {
        It(TEXT("获取对象的有效状态"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            AActor* Actor = World->SpawnActor(AActor::StaticClass());
            UnLua::PushUObject(L, Actor);
            lua_setglobal(L, "G_Actor");
            Actor->K2_DestroyActor();

            GEngine->ForceGarbageCollection(true);
            World->Tick(LEVELTICK_TimeOnly, SMALL_NUMBER);

            const char* Chunk = R"(
            return G_Actor:IsValid()
            )";
            UnLua::RunChunk(L, Chunk);

            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("GetName"), [this]()
    {
        It(TEXT("获取对象的名称"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Outer = NewObject(UE.UUnLuaTestStub)
            local Actor = NewObject(UE.AActor, Outer, 'Test')
            return Actor:GetName()
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tostring(L, -1), "Test");
        });
    });

    Describe(TEXT("GetOuter"), [this]()
    {
        It(TEXT("获取对象的外部对象"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local ParentActor = NewObject(UE.AActor)
            local ChildActor = NewObject(UE.AActor, ParentActor)
            return ChildActor:GetOuter(), ParentActor
            )";
            UnLua::RunChunk(L, Chunk);
            AActor* Actual = (AActor*)UnLua::GetUObject(L, -2);
            AActor* Expected = (AActor*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Expected, Actual);
        });
    });

    Describe(TEXT("GetClass"), [this]()
    {
        It(TEXT("获取对象的类型"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Actor = NewObject(UE.AActor)
            return Actor:GetClass()
            )";
            UnLua::RunChunk(L, Chunk);
            UClass* Class = (UClass*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Class, AActor::StaticClass());
        });
    });

    Describe(TEXT("GetWorld"), [this]()
    {
        It(TEXT("获取对象所在的World"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Actor = World:SpawnActor(UE.AActor)
            return Actor:GetWorld()
            )";
            UnLua::RunChunk(L, Chunk);
            UWorld* Actual = (UWorld*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Actual, World);
        });
    });

    Describe(TEXT("IsA"), [this]()
    {
        It(TEXT("判断对象是否为指定类型"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Actor = NewObject(UE.AActor)
            return Actor:IsA(UE.AActor), Actor:IsA(UE.UGameInstance), Actor:IsA(UE.UClass), UE.AActor:IsA(UE.UClass)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -4));
            TEST_FALSE(lua_toboolean(L, -3));
            TEST_FALSE(lua_toboolean(L, -2));
            TEST_TRUE(lua_toboolean(L, -1));
        });
    });

    xDescribe(TEXT("Release"), [this]()
    {
        It(TEXT("释放对象在LuaVM的引用"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            check(false);
        });
    });

    AfterEach([this]
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        UnLua::Shutdown();
    });
}

#endif
