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
#include "AITestsCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

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

struct UNLUATESTSUITE_API FUnLuaTestBase : FAITestBase
{
public:
    virtual bool SetUp() override;

    virtual bool Update() override;

    virtual void TearDown() override;

protected:
    virtual FString GetMapName() { return ""; }

    void AddLatent(TFunction<void()>&& Func, float Delay = 0.1f) const;

    lua_State* L;
};

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
