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

#pragma once

#include "UnLua.h"
#include "Engine.h"
#include "GameFramework/GameStateBase.h"

#if WITH_DEV_AUTOMATION_TESTS

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FUnLuaTestCommand_WaitSeconds, float, Duration);

class FUnLuaTestCommand_WaitOneTick : public IAutomationLatentCommand
{
public:
    FUnLuaTestCommand_WaitOneTick()
        : bAlreadyRun(false)
    {
    }

    virtual bool Update() override;
private:
    bool bAlreadyRun;
};

class FUnLuaTestDelayedCallbackLatentCommand : public IAutomationLatentCommand
{
public:
    FUnLuaTestDelayedCallbackLatentCommand(TFunction<void()>&& InCallback, float InDelay = 0.1f)
        : Callback(MoveTemp(InCallback)), Delay(InDelay)
    {
        //Set the start time
        StartTime = FPlatformTime::Seconds();
    }

    virtual bool Update() override
    {
        const double NewTime = FPlatformTime::Seconds();
        if (NewTime - StartTime >= static_cast<double>(Delay))
        {
            Callback();
            return true;
        }
        return false;
    }

private:
    TFunction<void()> Callback;
    float Delay;
};

class UNLUATESTSUITE_API FUnLuaTestCommand_LoadMap : public IAutomationLatentCommand
{
public:
    FUnLuaTestCommand_LoadMap(FWorldContext* WorldContext, FString MapName)
        : MapName(MapName), WorldContext(WorldContext), bMapLoadingStarted(false), bMapLoaded(false)
    {
    }

    virtual bool Update() override
    {
        if (!bMapLoadingStarted)
        {
            const auto OldWorld = GWorld;
            const FURL URL(*MapName);
            FString Error;
            LoadPackage(nullptr, *URL.Map, LOAD_None);
            FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
            GEngine->LoadMap(*WorldContext, URL, nullptr, Error);
            GWorld = OldWorld;
            bMapLoadingStarted = true;
            return false;
        }

        if (!bMapLoaded)
        {
            if (!bMapLoadingStarted)
                return false;

            const auto TestWorld = WorldContext->World();
            if (!TestWorld)
                return false;

            if (TestWorld && TestWorld->AreActorsInitialized())
            {
                AGameStateBase* GameState = TestWorld->GetGameState();
                if (GameState && GameState->HasMatchStarted())
                {
                    return true;
                }
            }

            return false;
        }

        return false;
    }

private:
    FString MapName;
    FWorldContext* WorldContext;
    bool bMapLoadingStarted;
    bool bMapLoaded;
};

struct UNLUATESTSUITE_API FUnLuaTestBase
{
public:
    virtual ~FUnLuaTestBase()
    {
    }

    virtual bool InstantTest() { return false; }

    virtual bool SetUp();

    virtual bool Update() { return true; }

    virtual void TearDown();

    virtual void SetTestRunner(FAutomationTestBase& AutomationTestInstance) { TestRunner = &AutomationTestInstance; }

    void AddLatent(TFunction<void()>&& Func, float Delay = 0.1f) const;

protected:
    FUnLuaTestBase() : L(nullptr), TestRunner(nullptr), GameInstance(nullptr), WorldContext(nullptr)
    {
    }

    virtual FString GetMapName() { return ""; }

    FAutomationTestBase& GetTestRunner() const
    {
        check(TestRunner);
        return *TestRunner;
    }

    UWorld* GetWorld() const;

    void SimulateTick(float Seconds = SMALL_NUMBER, ELevelTick TickType = LEVELTICK_All) const;

    void LoadMap(FString MapName) const;
    
    lua_State* L;
    FAutomationTestBase* TestRunner;
    UGameInstance* GameInstance;
    FWorldContext* WorldContext;
};

DEFINE_EXPORTED_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(UNLUATESTSUITE_API, FUnLuaTestCommand_SetUpTest, FUnLuaTestBase*, UnLuaTest);

DEFINE_EXPORTED_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(UNLUATESTSUITE_API, FUnLuaTestCommand_PerformTest, FUnLuaTestBase*, UnLuaTest);

DEFINE_EXPORTED_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(UNLUATESTSUITE_API, FUnLuaTestCommand_TearDownTest, FUnLuaTestBase*, UnLuaTest);

/////////////////////////////////////////
// TEST DEFINE MACROS

#define IMPLEMENT_UNLUA_LATENT_TEST(TestClass, PrettyName) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass##_Runner, PrettyName, (EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)) \
bool TestClass##_Runner::RunTest(const FString& Parameters) \
{ \
/* spawn test instance. Setup should be done in test's constructor */ \
TestClass* TestInstance = new TestClass(); \
TestInstance->SetTestRunner(*this); \
/* set up */ \
ADD_LATENT_AUTOMATION_COMMAND(FUnLuaTestCommand_SetUpTest(TestInstance)); \
/* run latent command to update */ \
ADD_LATENT_AUTOMATION_COMMAND(FUnLuaTestCommand_PerformTest(TestInstance)); \
/* run latent command to tear down */ \
ADD_LATENT_AUTOMATION_COMMAND(FUnLuaTestCommand_TearDownTest(TestInstance)); \
return true; \
}

#define IMPLEMENT_UNLUA_INSTANT_TEST(TestClass, PrettyName) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass##Runner, PrettyName, (EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)) \
bool TestClass##Runner::RunTest(const FString& Parameters) \
{ \
bool bSuccess = false; \
/* spawn test instance. */ \
TestClass* TestInstance = new TestClass(); \
TestInstance->SetTestRunner(*this); \
/* set up */ \
if (TestInstance->SetUp()) \
{ \
/* call the instant-test code */ \
bSuccess = TestInstance->InstantTest(); \
/* tear down */ \
TestInstance->TearDown(); \
}\
delete TestInstance; \
return bSuccess; \
}

#define IMPLEMENT_TESTSUITE_TEST(TestClass, PrettyName) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass, PrettyName, (EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter))

#define BEGIN_TESTSUITE(TestClass, PrettyName) \
namespace UnLuaTestSuite { \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass, PrettyName, (EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter))

#define END_TESTSUITE(TestClass) \
}

///////////////////////////////
// RUNNER_TEST MACROS

#define RUNNER_TEST_TRUE(expression)\
    if (!GetTestRunner().TestTrue(TEXT(#expression), (bool)(expression)))\
    {\
        return true;\
    }

#define RUNNER_TEST_FALSE(expression)\
    if (!GetTestRunner().TestFalse(TEXT(#expression), (bool)(expression)))\
    {\
    return true;\
    }

#define RUNNER_TEST_EQUAL(expression, expected)\
    if (!GetTestRunner().TestEqual(TEXT(#expression), expression, expected))\
    {\
    return true;\
    }

#define RUNNER_TEST_NULL(expression)\
    if (!GetTestRunner().TestNull(TEXT(#expression), expression))\
    {\
    return true;\
    }

#define RUNNER_TEST_NOT_NULL(expression)\
    if (!GetTestRunner().TestNotNull(TEXT(#expression), expression))\
    {\
    return true;\
    }

#endif //WITH_DEV_AUTOMATION_TESTS
