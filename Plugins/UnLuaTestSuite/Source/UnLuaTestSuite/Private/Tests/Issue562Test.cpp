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


#include "LowLevel.h"
#include "UnLuaCompatibility.h"
#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue562 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        // 使用-binnedmalloc2来提高复现概率

        const auto Chunk1 = R"(
            local InterfaceClass = UE.UClass.Load('/UnLuaTestSuite/Tests/Regression/Issue562/BPI_Issue562.BPI_Issue562_C')
            local Array = UE.TArray(InterfaceClass)
        )";
        UnLua::RunChunk(L, Chunk1);

        const auto Before = FindFirstObject<UClass>(TEXT("BPI_Issue562_C"));
        RUNNER_TEST_NOT_NULL(Before);

        CollectGarbage(RF_NoFlags, true);
        const auto After = FindFirstObject<UClass>(TEXT("BPI_Issue562_C"));
        RUNNER_TEST_NULL(After);

        lua_gc(L, LUA_GCCOLLECT);
        lua_gc(L, LUA_GCCOLLECT);

        // LogUObjectBase: Error: Object flags are invalid or either Class or Outer is misaligned

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue562, TEXT("UnLua.Regression.Issue562 容器元素为蓝图类型时，在蓝图Class被提前UE回收后，会导致容器在luagc时报错"))

#endif
