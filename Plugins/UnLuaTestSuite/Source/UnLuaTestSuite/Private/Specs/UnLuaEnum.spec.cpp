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

#include "UnLuaBase.h"
#include "UnLuaTemplate.h"
#include "Misc/AutomationTest.h"
#include "UnLuaTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaEnumSpec, "UnLua.API.Enum", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaEnumSpec)

void FUnLuaEnumSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("GetNameStringByValue"), [this]()
    {
        It(TEXT("获取枚举的名称"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            return UE.ECollisionResponse:GetNameStringByValue(UE.ECollisionResponse.ECR_Overlap)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = lua_tostring(L, -1);
            const auto Expected = UEnum::GetValueAsString(ECR_Overlap);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("不存在的枚举值返回nil"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int16>(0x7FFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int16>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FFFLL);
        });
    });

    Describe(TEXT("GetDisplayNameTextByValue"), [this]()
    {
        It(TEXT("获取枚举的本地化文本"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            return UE.ECollisionResponse:GetDisplayNameTextByValue(UE.ECollisionResponse.ECR_Overlap)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = UTF8_TO_TCHAR(lua_tostring(L, -1));
            const auto Expected = UEnum::GetDisplayValueAsText(ECR_Overlap).ToString();
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("不存在的枚举值返回nil"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int16>(0x7FFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int16>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FFFLL);
        });
    });
    
    Describe(TEXT("GetMaxValue"), [this]
    {
        It(TEXT("获取枚举类型的最大值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            return UE.ECollisionResponse:GetMaxValue()\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = (int32)lua_tointeger(L, -1);
            const auto Expected = (int32)ECR_MAX;
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
