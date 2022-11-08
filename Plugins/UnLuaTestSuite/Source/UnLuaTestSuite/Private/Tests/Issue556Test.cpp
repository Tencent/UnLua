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


#include "Issue556Test.h"
#include "LowLevel.h"
#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue556 : FUnLuaTestBase
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

        const auto Actor = (AIssue556Actor*)World->SpawnActor(AIssue556Actor::StaticClass());

        TArray<FHexHandle> AddHexHandles;
        AddHexHandles.Add({1});
        AddHexHandles.Add({2});
        AddHexHandles.Add({3});
        TArray<FHexHandle> RemoveHexHandles;
        RemoveHexHandles.Add({4});
        RemoveHexHandles.Add({5});

        Actor->PlayerViewChanged(AddHexHandles, RemoveHexHandles);
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue556, TEXT("UnLua.Regression.Issue556 Lua覆写的函数接收数组参数时可能无效"))

#endif
