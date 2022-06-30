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
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

bool FUnLuaTestCommand_WaitSeconds::Update()
{
    const float NewTime = FPlatformTime::Seconds();
    return NewTime - StartTime >= Duration;
}

bool FUnLuaTestCommand_WaitOneTick::Update()
{
    if (bAlreadyRun == false)
    {
        bAlreadyRun = true;
        return true;
    }
    return false;
}

bool FUnLuaTestCommand_SetUpTest::Update()
{
    return UnLuaTest && UnLuaTest->SetUp();
}

bool FUnLuaTestCommand_PerformTest::Update()
{
    return UnLuaTest == nullptr || UnLuaTest->Update();
}

bool FUnLuaTestCommand_TearDownTest::Update()
{
    if (!UnLuaTest)
        return false;

    UnLuaTest->AddLatent([this]
    {
        UnLuaTest->TearDown();
        delete UnLuaTest;
        UnLuaTest = nullptr;
    });
    return true;
}

bool FUnLuaTestBase::SetUp()
{
    UnLua::Startup();
    L = UnLua::GetState();

    GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    WorldContext = GameInstance->GetWorldContext();

    const auto& MapName = GetMapName();
    if (MapName.IsEmpty())
    {
        if (!WorldContext->World())
        {
            const auto World = UWorld::CreateWorld(EWorldType::Game, false, "UnLuaTest");
            World->SetGameInstance(GameInstance);
            WorldContext->SetCurrentWorld(World);
        }
    }
    else
    {
        if (InstantTest())
        {
            const auto OldWorld = GWorld;
            const FURL URL(*MapName);
            FString Error;
            LoadPackage(nullptr, *URL.Map, LOAD_None);
            FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
            GEngine->LoadMap(*WorldContext, URL, nullptr, Error);
            GWorld = OldWorld;    
        }
        else
        {
            AutomationOpenMap(MapName);
        }
    }

    return true;
}

void FUnLuaTestBase::TearDown()
{
    if (InstantTest())
    {
        const auto World = GetWorld();
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        UnLua::Shutdown();
    }
    else
    {
        AddLatent([this]
        {
            UnLua::Shutdown();
        });
    }
}

void FUnLuaTestBase::AddLatent(TFunction<void()>&& Func, float Delay) const
{
    ADD_LATENT_AUTOMATION_COMMAND(FUnLuaTestDelayedCallbackLatentCommand(MoveTemp(Func), Delay));
}

UWorld* FUnLuaTestBase::GetWorld() const
{
    return WorldContext->World();
}

void FUnLuaTestBase::SimulateTick(float Seconds, ELevelTick TickType) const
{
    constexpr auto Step = 1.0f / 60.0f;
    const auto World = GetWorld();
    while (Seconds > 0)
    {
        World->Tick(TickType, Step);
        Seconds -= Step;
    }
}

void FUnLuaTestBase::LoadMap(FString MapName) const
{
    const auto OldWorld = GWorld;
    const FURL URL(*MapName);
    FString Error;
    LoadPackage(nullptr, *URL.Map, LOAD_None);
    FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
    GEngine->LoadMap(*WorldContext, URL, nullptr, Error);
    GWorld = OldWorld;
}

void UnLuaTestSuite::PrintReferenceChain(UObject* Target)
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    FReferenceChainSearch Search(Target, EReferenceChainSearchMode::PrintAllResults | EReferenceChainSearchMode::FullChain);
#else
    FReferenceChainSearch Search(Target, EReferenceChainSearchMode::PrintAllResults);
#endif
}

#endif
