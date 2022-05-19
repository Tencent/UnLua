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

BEGIN_DEFINE_SPEC(FUnLuaBindingSpec, "UnLua.Binding", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUnLuaBindingSpec)

void FUnLuaBindingSpec::Define()
{
    BeforeEach([this]
    {
        Env = MakeShared<UnLua::FLuaEnv>();
        L = Env->GetMainState();
    });

    AfterEach([this]
    {
        Env.Reset();
        L = nullptr;
    });

    Describe(TEXT("Scoped Enum"), [this]
    {
        It(TEXT("Get"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
				local Stub = NewObject(UE.UUnLuaTestStub)
				return Stub.ScopedEnum
			)";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = (EScopedEnum::Type)lua_tointeger(L, -1);
            const auto Expected = EScopedEnum::Value1;
            TEST_EQUAL(Expected, Actual);
        });

        It(TEXT("Set"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
				local Stub = NewObject(UE.UUnLuaTestStub)
				Stub.ScopedEnum = UE.EScopedEnum.Value2
				return Stub.ScopedEnum
			)";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = (EScopedEnum::Type)lua_tointeger(L, -1);
            const auto Expected = EScopedEnum::Value2;
            TEST_EQUAL(Expected, Actual);
        });
    });

    Describe(TEXT("Enum Class"), [this]
    {
        It(TEXT("Get"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
				local Stub = NewObject(UE.UUnLuaTestStub)
				return Stub.EnumClass
			)";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = (EEnumClass)lua_tointeger(L, -1);
            const auto Expected = EEnumClass::Value1;
            TEST_EQUAL(Expected, Actual);
        });

        It(TEXT("Set"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
				local Stub = NewObject(UE.UUnLuaTestStub)
				Stub.EnumClass = UE.EEnumClass.Value2
				return Stub.EnumClass
			)";
            UnLua::RunChunk(L, Chunk);
            const auto Actual = (EEnumClass)lua_tointeger(L, -1);
            const auto Expected = EEnumClass::Value2;
            TEST_EQUAL(Expected, Actual);
        });
    });
}

#endif
