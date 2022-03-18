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
#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue328 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto World = GetWorld();

        const FURL URL;
        World->InitializeActorsForPlay(URL);
        World->BeginPlay();
        World->bBegunPlay = true;
        
        const auto ActorClass = LoadClass<AUnLuaTestActor>(World, TEXT("/UnLuaTestSuite/Tests/Regression/Issue328/BP_UnLuaTestActor_Issue328.BP_UnLuaTestActor_Issue328_C"));
        const auto Actor = (AUnLuaTestActor*)World->SpawnActor(ActorClass);
        const auto Result = Actor->TestForIssue328();
        RUNNER_TEST_TRUE(Result);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue328, TEXT("UnLua.Regression.Issue328 C++调用LUA覆写带返回值的函数会崩溃"))

#endif //WITH_DEV_AUTOMATION_TESTS
