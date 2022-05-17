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

BEGIN_DEFINE_SPEC(FUnLuaLibFVector2DSpec, "UnLua.API.FVector2D", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFVector2DSpec)

void FUnLuaLibFVector2DSpec::Define()
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

    Describe(TEXT("构造FVector2D"), [this]
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector2D()");
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(EForceInit::ForceInitToZero));
        });

        It(TEXT("分别指定X/Y"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector2D(1,2)");
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,2));
        });

        It(TEXT("同时指定X/Y"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector2D(1)");
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D()
            Vector:Set(1)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,0));
        });

        It(TEXT("设置X/Y"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D()
            Vector:Set(1,2)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,2));
        });
    });

    Describe(TEXT("Normalize"), [this]
    {
        It(TEXT("大于等于容差，归一化"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D(3,3)
            Vector:Normalize()
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            auto Expected = FVector2D(3, 3);
            Expected.Normalize();
            TEST_EQUAL(Vector, Expected);
        });
        It(TEXT("小于容差，不归一化"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D(3,3)
            Vector:Normalize(100)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            auto Expected = FVector2D(3, 3);
            Expected.Normalize(100);
            TEST_EQUAL(Vector, Expected);
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("向量相加"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector1 = UE.FVector2D(1,2)
            local Vector2 = UE.FVector2D(4,5)
            return Vector1 + Vector2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,2) + FVector2D(4,5));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("向量相减"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector1 = UE.FVector2D(1,2)
            local Vector2 = UE.FVector2D(4,5)
            return Vector1 - Vector2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,2) - FVector2D(4,5));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("向量乘以数字"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D(1,2)
            return Vector * 3
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(1,2) * 3);
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("向量除以数字"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D(2,4)
            return Vector / 2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(2,4) / 2);
        });
    });

    Describe(TEXT("unm"), [this]
    {
        It(TEXT("取反向量"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector2D(1,2)
            return -Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector2D>(L, -1, UnLua::TType<FVector2D>());
            TEST_EQUAL(Vector, FVector2D(-1,-2));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return tostring(UE.FVector2D(1,2))");
            const auto& Actual = lua_tostring(L, -1);
            const auto& Expected = FVector2D(1, 2).ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
