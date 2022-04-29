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

BEGIN_DEFINE_SPEC(FUnLuaLibFVectorSpec, "UnLua.API.FVector", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFVectorSpec)

void FUnLuaLibFVectorSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造FVector"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FVector()");
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(EForceInit::ForceInitToZero));
        });

        It(TEXT("分别指定X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FVector(1.1,2.2,3.3)");
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,3.3f));
        });

        It(TEXT("同时指定X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FVector(1.1)");
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector()\
            Vector:Set(1.1)\
            return Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,0,0));
        });

        It(TEXT("设置X/Y"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector()\
            Vector:Set(1.1,2.2)\
            return Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,0));
        });

        It(TEXT("设置X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector()\
            Vector:Set(1.1,2.2,3.3)\
            return Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,3.3f));
        });
    });

    Describe(TEXT("Normalize"), [this]
    {
        It(TEXT("大于等于容差，归一化并返回true"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector(3.3,3.3,3.3)\
            local Result = Vector:Normalize()\
            return Result, Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -2));
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            auto Expected = FVector(3.3f, 3.3f, 3.3f);
            Expected.Normalize();
            TEST_EQUAL(Vector, Expected);
        });
        It(TEXT("小于容差，直接返回false"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector(3.3,3.3,3.3)\
            local Result = Vector:Normalize(100)\
            return Result, Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -2));
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            auto Expected = FVector(3.3f, 3.3f, 3.3f);
            Expected.Normalize(100);
            TEST_EQUAL(Vector, Expected);
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("向量相加"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector1 = UE.FVector(1.1,2.2,3.3)\
            local Vector2 = UE.FVector(4.4,5.5,6.6)\
            return Vector1 + Vector2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,3.3f) + FVector(4.4f,5.5f,6.6f));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("向量相减"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector1 = UE.FVector(1.1,2.2,3.3)\
            local Vector2 = UE.FVector(4.4,5.5,6.6)\
            return Vector1 - Vector2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,3.3f) - FVector(4.4f,5.5f,6.6f));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("向量乘以浮点数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector(1.1,2.2,3.3)\
            return Vector * 2.1\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(1.1f,2.2f,3.3f) * 2.1f);
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("向量除以浮点数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector(2.2,4.4,8.8)\
            return Vector / 2.2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(2.2f,4.4f,8.8f) / 2.2f);
        });
    });

    Describe(TEXT("unm"), [this]
    {
        It(TEXT("取反向量"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FVector(1.1,2.2,3.3)\
            return -Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FVector>(L, -1, UnLua::TType<FVector>());
            TEST_EQUAL(Vector, FVector(-1.1f,-2.2f,-3.3f));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE.FVector(1,2,3))");
            const auto& Actual = lua_tostring(L, -1);
            const auto& Expected = FVector(1, 2, 3).ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
