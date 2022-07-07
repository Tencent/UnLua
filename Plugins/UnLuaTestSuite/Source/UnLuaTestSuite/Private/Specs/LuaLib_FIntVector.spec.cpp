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

BEGIN_DEFINE_SPEC(FUnLuaLibFIntVectorSpec, "UnLua.API.FIntVector", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFIntVectorSpec)

void FUnLuaLibFIntVectorSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造FIntVector"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FIntVector()");
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(EForceInit::ForceInitToZero));
        });

        It(TEXT("分别指定X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FIntVector(1,2,3)");
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,2,3));
        });

        It(TEXT("同时指定X/Y/Z"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FIntVector(1)");
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector()\
            Vector:Set(1)\
            return Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,0,0));
        });

        It(TEXT("设置X/Y"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector()\
            Vector:Set(1,2)\
            return Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,2,0));
        });
    });

    Describe(TEXT("SizeSquared"), [this]
    {
        It(TEXT("获取平方后的长度"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector(3,4,0)\
            return Vector:SizeSquared()\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& SizeSquared = (int32)lua_tointeger(L, -1);
            const auto& Size = FIntVector(3, 4, 0).Size();
            TEST_EQUAL(SizeSquared, Size * Size);
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("向量相加"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector1 = UE.FIntVector(1,2,3)\
            local Vector2 = UE.FIntVector(4,5,6)\
            return Vector1 + Vector2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,2,3) + FIntVector(4,5,6));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("向量相减"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector1 = UE.FIntVector(1,2,3)\
            local Vector2 = UE.FIntVector(4,5,6)\
            return Vector1 - Vector2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,2,3) - FIntVector(4,5,6));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("向量乘以整数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector(1,2,3)\
            return Vector * 2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(1,2,3) * 2);
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("向量除以整数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector(2,4,8)\
            return Vector / 2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(2,4,8) / 2);
        });
    });

    Describe(TEXT("unm"), [this]
    {
        It(TEXT("取反向量"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Vector = UE.FIntVector(1,2,3)\
            return -Vector\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Vector = UnLua::Get<FIntVector>(L, -1, UnLua::TType<FIntVector>());
            TEST_EQUAL(Vector, FIntVector(-1,-2,-3));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE.FIntVector(1,2,3))");
            const auto& Actual =lua_tostring(L, -1);
            const auto& Expected =  FIntVector(1,2,3).ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
