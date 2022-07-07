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

BEGIN_DEFINE_SPEC(FUnLuaLibFQuatSpec, "UnLua.API.FQuat", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFQuatSpec)

void FUnLuaLibFQuatSpec::Define()
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

    Describe(TEXT("构造FQuat"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            Env->DoString("return UE.FQuat()");
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(ForceInitToZero);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("分别指定X/Y/Z/W"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            Env->DoString("return UE.FQuat(4,3,2,1)");
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 3, 2, 1);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("指定轴与角度"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Axis = UE.FVector(0,0,1)
            local Quat = UE.FQuat.FromAxisAndAngle(Axis, 3.14)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(FVector(0, 0, 1), 3.14f);
            TEST_QUAT_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat()
            Quat:Set(4)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 0, 0, 0);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("设置X/Y"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat()
            Quat:Set(4,3)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 3, 0, 0);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("设置X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat()
            Quat:Set(4,3,2)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 3, 2, 0);
            TEST_EQUAL(Actual, Expected);
        });

        It(TEXT("设置X/Y/Z/W"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat()
            Quat:Set(4,3,2,1)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 3, 2, 1);
            TEST_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("Normalize"), [this]
    {
        It(TEXT("大于等于容差，归一化"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat(4,3,2,1)
            Quat:Normalize()
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            auto Expected = FQuat(4, 3, 2, 1);
            Expected.Normalize();
            TEST_EQUAL(Expected, Actual);
        });

        It(TEXT("小于容差，不归一化"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat(4,3,2,1)
            Quat:Normalize(100)
            return Quat
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            auto Expected = FQuat(4, 3, 2, 1);
            Expected.Normalize(100);
            TEST_EQUAL(Expected, Actual);
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("四元数乘以浮点数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            local Quat = UE.FQuat(4,3,2,1)
            return Quat * 2.0
            )";
            Env->DoString(Chunk);
            const auto& Actual = UnLua::Get<FQuat>(L, -1, UnLua::TType<FQuat>());
            const auto& Expected = FQuat(4, 3, 2, 1) * 2.0f;
            TEST_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            Env->DoString("return tostring(UE.FQuat(4,3,2,1))");
            const auto& Actual = lua_tostring(L, -1);
            const auto& Expected = FQuat(4, 3, 2, 1).ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });
}

#endif
