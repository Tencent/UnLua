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

#include "UnLuaCompatibility.h"
#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue585 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        // 正常加载Enum
        const auto Chunk1 = R"(
            G_Enum = UE.UObject.Load("/UnLuaTestSuite/Tests/Misc/Enum_UserDefined.Enum_UserDefined")
            print("G_Enum.A=", G_Enum.A)
        )";
        UnLua::RunChunk(L, Chunk1);

        // 无引用被GC回收
        CollectGarbage(RF_NoFlags, true);
        RUNNER_TEST_NULL(FindFirstObject<UEnum>(TEXT("Enum_UserDefined")));

        // 被回收后能够自动加载
        const auto Chunk2 = R"(
            print("G_Enum.A=", G_Enum.A)
        )";
        UnLua::RunChunk(L, Chunk2);
        
        // 无引用被GC回收
        CollectGarbage(RF_NoFlags, true);
        RUNNER_TEST_NULL(FindFirstObject<UEnum>(TEXT("Enum_UserDefined")));

        // 支持UnLua.Ref来持有
        const auto Chunk3 = R"(
            G_Enum = UE.UObject.Load("/UnLuaTestSuite/Tests/Misc/Enum_UserDefined.Enum_UserDefined")
            UnLua.Ref(G_Enum)
        )";
        UnLua::RunChunk(L, Chunk3);
        
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue585, TEXT("UnLua.Regression.Issue585 蓝图枚举无法被UnLuaRef"))

#endif
