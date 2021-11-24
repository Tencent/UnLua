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
#include "GameFramework/GameStateBase.h"

#if WITH_DEV_AUTOMATION_TESTS

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FUnLuaTestCommand_WaitSeconds, float, Duration);

static UWorld* GetAnyGameWorld()
{
    UWorld* TestWorld = nullptr;
    const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
    for (const FWorldContext& Context : WorldContexts)
    {
        if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != NULL)
        {
            TestWorld = Context.World();
            break;
        }
    }

    return TestWorld;
}

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
    FUnLuaTestCommand_LoadMap(FString MapName)
        : MapName(MapName), bMapLoadingStarted(false), bMapLoaded(false)
    {
    }

    virtual bool Update() override
    {
        if (!bMapLoadingStarted)
        {
            const auto TestWorld = GetAnyGameWorld();
            if (!TestWorld)
                return false;

            bMapLoadingStarted = true;
            // Convert both to short names and strip PIE prefix
            FString ShortMapName = FPackageName::GetShortName(MapName);
            FString ShortWorldMapName = FPackageName::GetShortName(TestWorld->GetMapName());

            if (TestWorld->GetOutermost()->PIEInstanceID != INDEX_NONE)
            {
                FString PIEPrefix = FString::Printf(PLAYWORLD_PACKAGE_PREFIX TEXT("_%d_"), TestWorld->GetOutermost()->PIEInstanceID);
                ShortWorldMapName.ReplaceInline(*PIEPrefix, TEXT(""));
            }
            if (ShortMapName != ShortWorldMapName)
            {
                FString OpenCommand = FString::Printf(TEXT("Open %s"), *MapName);
                GEngine->Exec(TestWorld, *OpenCommand);
            }
            return false;
        }

        if (!bMapLoaded)
        {
            const auto TestWorld = GetAnyGameWorld();
            if (!TestWorld)
                return false;

            if (!TestWorld->AreActorsInitialized())
                return false;

            AGameStateBase* GameState = TestWorld->GetGameState();
            if (GameState && GameState->HasMatchStarted())
                return true;
            
            return false;
        }

        return false;
    }

private:
    FString MapName;
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

protected:
    FUnLuaTestBase() : L(nullptr), TestRunner(nullptr)
    {
    }

    virtual FString GetMapName() { return ""; }

    FAutomationTestBase& GetTestRunner() const
    {
        check(TestRunner);
        return *TestRunner;
    }

    void AddLatent(TFunction<void()>&& Func, float Delay = 0.1f) const;

    UWorld* CreateWorld(FName WorldName = "UnLuaTest");

    lua_State* L;
    FAutomationTestBase* TestRunner;
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
