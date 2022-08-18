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
#if ENABLE_TYPE_CHECK

struct FUnLuaTest_Issue293 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        GetTestRunner().AddExpectedError(TEXT("Invalid parameter"), EAutomationExpectedErrorFlags::Contains);
        const char* Chunk1 = "\
            return UE.UUnLuaTestFunctionLibrary.TestForIssue293('A', 1)\
            ";
        UnLua::RunChunk(L, Chunk1);

        const char* Chunk2 = "\
            return UE.UUnLuaTestFunctionLibrary.TestForIssue293('A', 1, UE.TArray(UE.FColor))\
            ";
        UnLua::RunChunk(L, Chunk2);
        const auto Actual = (int32)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(Actual, 0);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue293, TEXT("UnLua.Regression.Issue293 关闭RPC会导致部分函数在非Editor模式下crash"))

#endif
#endif //WITH_DEV_AUTOMATION_TESTS
