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
#include "Misc/AutomationTest.h"


#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue276 : FUnLuaTestBase
{
    virtual FString GetMapName() override { return TEXT("/Game/Tests/Regression/Issue276/Issue276"); }

    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        SimulateTick(1.0f);
        lua_getglobal(L, "Flag");
        const bool Flag = !!lua_toboolean(L, -1);
        RUNNER_TEST_TRUE(Flag);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue276, TEXT("UnLua.Regression.Issue276 游戏世界暂停后，coroutine delay 功能不工作"))

#endif //WITH_DEV_AUTOMATION_TESTS
