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
        L = UnLua::CreateState();
    });

    Describe(TEXT("Cpp Enum"), [this]()
    {
        Describe(TEXT("GetValue"), [this]()
        {
            It(TEXT("获取枚举的值"), EAsyncExecution::ThreadPool, [this]()
            {
                const char* Chunk = "\
                return UE.EBlendMode.BLEND_Masked\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL((int32)lua_tointeger(L, -1), 1);
            });

            It(TEXT("不存在的枚举值返回-1"), EAsyncExecution::ThreadPool, [this]()
            {
                const char* Chunk = "\
                return UE.EBlendMode.NotExists\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL((int32)lua_tonumber(L, -1), -1);
            });
        });

        Describe(TEXT("GetNameByValue"), [this]()
        {
            It(TEXT("获取枚举的名称"), EAsyncExecution::ThreadPool, [this]()
            {
                const char* Chunk = "\
                return UE.ECollisionResponse:GetNameByValue(UE.ECollisionResponse.ECR_Overlap)\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL(lua_tostring(L, -1), "Overlap");
            });

            It(TEXT("不存在的枚举值返回空字符串"), EAsyncExecution::ThreadPool, [this]()
            {
                const char* Chunk = "\
                return UE.ECollisionResponse:GetNameByValue(9999)\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL(lua_tostring(L, -1), "");
            });
        });

        Describe(TEXT("GetMaxValue"), [this]
        {
            It(TEXT("获取枚举类型的最大值"), EAsyncExecution::ThreadPool, [this]()
            {
                const char* Chunk = "\
                return UE.ECollisionResponse:GetMaxValue()\
                ";
                UnLua::RunChunk(L, Chunk);
                const auto Actual = (int32)lua_tointeger(L, -1);
                const auto Expected = (int32)ECollisionResponse::ECR_MAX;
                TEST_EQUAL(Actual, Expected);
            });
        });
    });

    Describe(TEXT("User Defined Enum"), [this]()
    {
        Describe(TEXT("GetValue"), [this]()
        {
            It(TEXT("自定义枚举的数值Value实际等同于Index"), EAsyncExecution::TaskGraphMainThread, [this]()
            {
                const char* Chunk = "\
                local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
                return Enum.A\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL((int32)lua_tointeger(L, -1), 0);
            });

            It(TEXT("不存在的枚举值返回-1"), EAsyncExecution::TaskGraphMainThread, [this]()
            {
                const char* Chunk = "\
                local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
                return Enum.NotExists\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL((int32)lua_tonumber(L, -1), -1);
            });
        });

        Describe(TEXT("GetNameByValue"), [this]()
        {
            It(TEXT("获取枚举的名称"), EAsyncExecution::TaskGraphMainThread, [this]()
            {
                const char* Chunk = "\
                local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
                return Enum:GetNameByValue('A')\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL(lua_tostring(L, -1), "A");
            });

            It(TEXT("不存在的枚举值返回空字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
            {
                const char* Chunk = "\
                local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
                return Enum:GetNameByValue(9999)\
                ";
                UnLua::RunChunk(L, Chunk);
                TEST_EQUAL(lua_tostring(L, -1), "");
            });
        });

        Describe(TEXT("GetMaxValue"), [this]
        {
            It(TEXT("获取枚举类型的最大值"), EAsyncExecution::TaskGraphMainThread, [this]()
            {
                const char* Chunk = "\
                local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
                return Enum:GetMaxValue()\
                ";
                UnLua::RunChunk(L, Chunk);
                const auto Actual = (int32)lua_tointeger(L, -1);
                const auto Expected = 2;
                TEST_EQUAL(Actual, Expected);
            });
        });

        It(TEXT("直接访问UE XXX，和UObject Load的对象为同一个"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Enum = UE.UObject.Load('/Game/Tests/Misc/Enum_UserDefined')\
            return Enum == UE.Enum_UserDefined\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Result = (bool)lua_toboolean(L, -1);
            TEST_TRUE(Result);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
