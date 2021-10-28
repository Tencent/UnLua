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
#include "Specs/TestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibFIntPointSpec, "UnLua.API.FIntPoint", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFIntPointSpec)

void FUnLuaLibFIntPointSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("构造FIntPoint"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::ThreadPool, [this]()
        {
            UnLua::RunChunk(L, "return UE4.FIntPoint()");
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(EForceInit::ForceInit));
        });

        It(TEXT("分别指定X和Y"), EAsyncExecution::ThreadPool, [this]()
        {
            UnLua::RunChunk(L, "return UE4.FIntPoint(1,2)");
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(1,2));
        });

        It(TEXT("同时指定X和Y"), EAsyncExecution::ThreadPool, [this]()
        {
            UnLua::RunChunk(L, "return UE4.FIntPoint(1)");
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(1));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置X"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point = UE4.FIntPoint()\
            Point:Set(1)\
            return Point\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(1,0));
        });

        It(TEXT("设置XY"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point = UE4.FIntPoint()\
            Point:Set(1,2)\
            return Point\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(1,2));
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("两点相加"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point1 = UE4.FIntPoint(2,4)\
            local Point2 = UE4.FIntPoint(1,2)\
            return Point1 + Point2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(2,4) + FIntPoint(1,2));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("两点相减"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point1 = UE4.FIntPoint(2,4)\
            local Point2 = UE4.FIntPoint(1,2)\
            return Point1 - Point2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(2,4) - FIntPoint(1,2));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("两点相乘"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point1 = UE4.FIntPoint(1,2)\
            local Point2 = UE4.FIntPoint(3,4)\
            return Point1 * Point2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(1,2) * FIntPoint(3,4));
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("两点相除"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point1 = UE4.FIntPoint(2,4)\
            local Point2 = UE4.FIntPoint(1,2)\
            return Point1 / Point2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(2,4) / FIntPoint(1,2));
        });
    });

    Describe(TEXT("unm"), [this]
    {
        It(TEXT("取相反数"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Point = UE4.FIntPoint(1,2)\
            return -Point\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Point = UnLua::Get<FIntPoint>(L, -1, UnLua::TType<FIntPoint>());
            TEST_EQUAL(Point, FIntPoint(-1,-2));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::ThreadPool, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE4.FIntPoint(1,2))");
            const auto S = lua_tostring(L, -1); 
            TEST_EQUAL(lua_tostring(L, -1), "X=1 Y=2");
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
