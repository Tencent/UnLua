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

BEGIN_DEFINE_SPEC(FUnLuaLibFVector4Spec, "UnLua.API.FVector4", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFVector4Spec)

void FUnLuaLibFVector4Spec::Define()
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

    Describe(TEXT("构造FVector4"), [this]
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector4()");
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(EForceInit::ForceInitToZero));
        });

        It(TEXT("指定X"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector4(1)");
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1));
        });

        It(TEXT("指定X/Y"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector4(1,2)");
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2));
        });

        It(TEXT("指定X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector4(1,2,3)");
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3));
        });

        It(TEXT("指定X/Y/Z/W"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return UE.FVector4(1,2,3,4)");
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,4));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4()
            Vector:Set(1)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,0,0,0));
        });

        It(TEXT("设置X/Y"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4()
            Vector:Set(1,2)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,0,0));
        });

        It(TEXT("设置X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4()
            Vector:Set(1,2,3)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,0));
        });

        It(TEXT("设置X/Y/Z/W"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4()
            Vector:Set(1,2,3,4)
            return Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,4));
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("向量相加"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector1 = UE.FVector4(1,2,3,4)
            local Vector2 = UE.FVector4(5,6,7,8)
            return Vector1 + Vector2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,4) + FVector4(5,6,7,8));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("向量相减"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector1 = UE.FVector4(1,2,3,4)
            local Vector2 = UE.FVector4(5,6,7,8)
            return Vector1 - Vector2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,4) - FVector4(5,6,7,8));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("向量乘"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4(1,2,3,4)
            return Vector * 2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            TEST_EQUAL(Vector, FVector4(1,2,3,4) * 2);
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("向量除"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4(2,2,4,8)
            return Vector / 2
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            const auto& Expected = FVector4(2, 2, 4, 8) / 2;
            TEST_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("unm"), [this]
    {
        It(TEXT("取反向量"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Vector = UE.FVector4(1,2,3,4)
            return -Vector
            )";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector4>(L, -1, UnLua::TType<FVector4>());
            const auto Expected = -FVector4(1, 2, 3, 4);
            TEST_EQUAL(Vector, Expected);
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            UnLua::RunChunk(L, "return tostring(UE.FVector4(1,2,3,4))");
            const auto& Actual = lua_tostring(L, -1);
            const auto& Expected = FVector4(1, 2, 3, 4).ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
