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
#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue300 : FUnLuaTestBase
{
    virtual FString GetMapName() override { return TEXT("/Game/Tests/Regression/Issue300/Issue300"); }

    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        lua_getglobal(L, "Result1");
        const auto Result1 = static_cast<int32>(lua_tonumber(L, -1));

        lua_getglobal(L, "Result2");
        const auto Result2 = static_cast<int32>(lua_tonumber(L, -1));

        RUNNER_TEST_EQUAL(Result1, 10);
        RUNNER_TEST_EQUAL(Result2, 10);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue300, TEXT("UnLua.Regression.Issue300 LUA覆写的函数返回给蓝图的值错误"))

#endif //WITH_DEV_AUTOMATION_TESTS
