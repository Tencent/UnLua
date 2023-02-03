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
#include "GameFramework/DefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Engine.h"
#include "UnLuaTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibWorldSpec, "UnLua.API.World", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
    UWorld* World;
END_DEFINE_SPEC(FUnLuaLibWorldSpec)

void FUnLuaLibWorldSpec::Define()
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

    Describe(TEXT("SpawnActor"), [this]
    {
        It(TEXT("创建Actor，仅指定Actor类型"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Actor = World:SpawnActor(UE.AActor)
            return Actor
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Actor = (AActor*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Actor->GetClass(), AActor::StaticClass());
        });

        It(TEXT("创建Actor，指定初始Transform"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Rotation = UE.FQuat(4,3,2,1)
            local Translation = UE.FVector(1,1,1)
            local Scale = UE.FVector(2,2,2)
            local Transform = UE.FTransform(Rotation, Translation, Scale)
            local Actor = World:SpawnActor(UE.ACharacter, Transform)
            return Actor
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Actor = (AActor*)UnLua::GetUObject(L, -1);
            const auto& Transform = Actor->GetActorTransform();
            TEST_QUAT_EQUAL(Transform.GetRotation(), FQuat(4,3,2,1));
            TEST_EQUAL(Transform.GetTranslation(), FVector(1,1,1));
            TEST_EQUAL(Transform.GetScale3D(), FVector(2,2,2));
        });

        It(TEXT("创建Actor，指定初始Transform/CollisionHandling"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Rotation = UE.FQuat(4,3,2,1)
            local Translation = UE.FVector(1,1,1)
            local Scale = UE.FVector(2,2,2)
            local Transform = UE.FTransform(Rotation, Translation, Scale)
            local Actor = World:SpawnActor(UE.ACharacter, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn)
            return Actor
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Actor = (AActor*)UnLua::GetUObject(L, -1);
            const auto& Transform = Actor->GetActorTransform();
            TEST_QUAT_EQUAL(Transform.GetRotation(), FQuat(4,3,2,1));
            TEST_EQUAL(Transform.GetTranslation(), FVector(1,1,1));
            TEST_EQUAL(Transform.GetScale3D(), FVector(2,2,2));
        });

        It(TEXT("创建Actor，指定初始Transform/CollisionHanding/Owner"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto OwnerActor = World->SpawnActor(AActor::StaticClass());
            UnLua::PushUObject(L, OwnerActor);
            lua_setglobal(L, "G_OwnerActor");

            const auto Chunk = R"(
            local Rotation = UE.FQuat(4,3,2,1)
            local Translation = UE.FVector(1,1,1)
            local Scale = UE.FVector(2,2,2)
            local Transform = UE.FTransform(Rotation, Translation, Scale)
            local Actor = World:SpawnActor(UE.ACharacter, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, G_OwnerActor)
            return Actor
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Actor = (AActor*)UnLua::GetUObject(L, -1);
            const auto& Transform = Actor->GetActorTransform();
            TEST_QUAT_EQUAL(Transform.GetRotation(), FQuat(4,3,2,1));
            TEST_EQUAL(Transform.GetTranslation(), FVector(1,1,1));
            TEST_EQUAL(Transform.GetScale3D(), FVector(2,2,2));
            TEST_EQUAL(Actor->GetOwner(), OwnerActor);
        });

        It(TEXT("创建Actor，指定初始Transform/CollisionHanding/Owner/Instigator"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto OwnerActor = World->SpawnActor(AActor::StaticClass());
            UnLua::PushUObject(L, OwnerActor);
            lua_setglobal(L, "G_OwnerActor");

            const auto InstigatorPawn = (APawn*)World->SpawnActor(ADefaultPawn::StaticClass());
            UnLua::PushUObject(L, InstigatorPawn);
            lua_setglobal(L, "G_InstigatorPawn");

            const auto Chunk = R"(
            local Rotation = UE.FQuat(4,3,2,1)
            local Translation = UE.FVector(1,1,1)
            local Scale = UE.FVector(2,2,2)
            local Transform = UE.FTransform(Rotation, Translation, Scale)
            local Actor = World:SpawnActor(UE.ACharacter, Transform, UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn, G_OwnerActor, G_InstigatorPawn)
            return Actor
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Actor = (AActor*)UnLua::GetUObject(L, -1);
            const auto& Transform = Actor->GetActorTransform();
            TEST_QUAT_EQUAL(Transform.GetRotation(), FQuat(4,3,2,1));
            TEST_EQUAL(Transform.GetTranslation(), FVector(1,1,1));
            TEST_EQUAL(Transform.GetScale3D(), FVector(2,2,2));
            TEST_EQUAL(Actor->SpawnCollisionHandlingMethod, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            TEST_EQUAL(Actor->GetOwner(), OwnerActor);
            TEST_EQUAL(Actor->GetInstigator(), InstigatorPawn);
        });

        xIt(TEXT("创建Actor，指定初始Transform/CollisionHanding/Owner/Instigator/LuaModule"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            check(false);
        });

        xIt(TEXT("创建Actor，自定义构造参数"), EAsyncExecution::TaskGraphMainThread, [this]
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

#endif //WITH_DEV_AUTOMATION_TESTS
