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

BEGIN_DEFINE_SPEC(FUnLuaBaseSpec, "UnLua.API.Base", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaBaseSpec)

void FUnLuaBaseSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("UnLua::Push"), [this]()
    {
        It(TEXT("正确传入int8到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int8>(0x7F));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int8>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FLL);
        });

        It(TEXT("正确传入int16到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int16>(0x7FFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int16>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FFFLL);
        });

        It(TEXT("正确传入int32到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int32>(0x7FFFFFFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int32>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FFFFFFFLL);
        });

        It(TEXT("正确传入int64到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<int64>(0x7FFFFFFFFFFFFFFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<int64>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0x7FFFFFFFFFFFFFFF);
        });

        It(TEXT("正确传入uint8到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<uint8>(0xFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<uint8>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0xFFLL);
        });

        It(TEXT("正确传入uint16到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<uint16>(0xFFFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<uint16>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0xFFFFLL);
        });

        It(TEXT("正确传入uint32到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<uint32>(0xFFFFFFFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<uint32>()));
            TEST_EQUAL(lua_tointeger(L, -1), 0xFFFFFFFFLL);
        });

        It(TEXT("正确传入uint64到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<uint64>(0xFFFFFFFFFFFFFFFF));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<uint64>()));
            TEST_EQUAL((int64)lua_tointeger(L, -1), (int64)0xFFFFFFFFFFFFFFFFLL);
        });

        It(TEXT("正确传入float到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, 0.123f);
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<float>()));
            TEST_EQUAL(lua_tonumber(L, -1), 0.123);
        });

        It(TEXT("正确传入double到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, 0.123);
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<double>()));
            TEST_EQUAL(lua_tonumber(L, -1), 0.123);
        });

        It(TEXT("正确传入bool到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, true);
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<bool>()));
            TEST_EQUAL(lua_toboolean(L, -1), true);
        });

        It(TEXT("正确传入const char*到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, "A");
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<const char*>()));
            TEST_EQUAL(lua_tostring(L, -1), "A");
        });

        It(TEXT("正确传入FString到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, FString("Hello"));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<FString>()));
            TEST_EQUAL(lua_tostring(L, -1), "Hello");
        });

        It(TEXT("正确传入中文FString到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, FString(TEXT("虚幻引擎")));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<FString>()));
            TEST_EQUAL(lua_tostring(L, -1), "虚幻引擎");
        });

        It(TEXT("正确传入FName到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, FName("Foo"));
            TEST_TRUE(UnLua::IsType(L, -1, UnLua::TType<FName>()));
            TEST_EQUAL(lua_tostring(L, -1), "Foo");
        });

        It(TEXT("正确传入void*到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::Push(L, static_cast<void*>(L));
            TEST_EQUAL((SIZE_T)lua_topointer(L, -1), (SIZE_T)L);
        });

        It(TEXT("正确传入struct到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            FVector VectorValue(1, 2, 3);
            UnLua::Push<FVector>(L, &VectorValue);
            TEST_EQUAL(UnLua::Get(L, -1, UnLua::TType<FVector>()), VectorValue);
        });

        It(TEXT("正确传入UObject到Lua堆栈"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            auto Expected = NewObject<AActor>(GEngine);
            UnLua::PushUObject(L, Expected);

            auto Actual = (AActor*)UnLua::GetUObject(L, -1);
            TEST_EQUAL(Expected, Actual);
        });
    });

    Describe(TEXT("UnLua::RunChunk"), [this]
    {
        It(TEXT("执行成功返回true"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            TEST_TRUE(UnLua::RunChunk(L, "return 123"));
        });

        It(TEXT("执行失败返回false"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            AddExpectedError(TEXT("syntax error"), EAutomationExpectedErrorFlags::Contains);
            TEST_FALSE(UnLua::RunChunk(L, "invalid chunk"));
        });
    });

    Describe(TEXT("UnLua::Call"), [this]
    {
        It(TEXT("调用成功，返回正确的返回值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "function GlobalFunction(v) return v end");
            const UnLua::FLuaRetValues RetValues = UnLua::Call(L, "GlobalFunction", "Foo");
            TEST_TRUE(RetValues.IsValid());
            TEST_EQUAL(RetValues.Num(), 1);
            TEST_EQUAL(RetValues[0].GetType(), LUA_TSTRING);
            TEST_EQUAL(RetValues[0].Value<const char*>(), "Foo");
        });

        It(TEXT("支持多参数传入和传出"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "function GlobalFunction(a,b,c,d) return a,b,c,d end");
            const UnLua::FLuaRetValues RetValues = UnLua::Call(L, "GlobalFunction", "A", 1, true, FVector(1, 2, 3));
            TEST_TRUE(RetValues.IsValid());
            TEST_EQUAL(RetValues.Num(), 4);
            TEST_EQUAL(RetValues[0].GetType(), LUA_TSTRING);
            TEST_EQUAL(RetValues[0].Value<const char*>(), "A");
            TEST_EQUAL(RetValues[1].GetType(), LUA_TNUMBER);
            TEST_EQUAL(RetValues[1].Value<int32>(), 1);
            TEST_EQUAL(RetValues[2].GetType(), LUA_TBOOLEAN);
            TEST_EQUAL(RetValues[2].Value<bool>(), true);
            TEST_EQUAL(RetValues[3].GetType(), LUA_TUSERDATA);
            TEST_EQUAL(RetValues[3].Value<FVector>(), FVector(1,2,3));
        });

        It(TEXT("调用失败，返回值被标记为无效"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            AddExpectedError(TEXT("Global function NotExistsFunction doesn't exist!"));
            const UnLua::FLuaRetValues RetValues = UnLua::Call(L, "NotExistsFunction", "Foo");
            TEST_FALSE(RetValues.IsValid());
        });
    });

    Describe(TEXT("UnLua::CallTableFunc"), [this]
    {
        It(TEXT("调用成功，返回正确的返回值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "GlobalTable = {}; function GlobalTable.Func(v) return v end");
            const UnLua::FLuaRetValues RetValues = UnLua::CallTableFunc(L, "GlobalTable", "Func", "Foo");
            TEST_TRUE(RetValues.IsValid());
            TEST_EQUAL(RetValues.Num(), 1);
            TEST_EQUAL(RetValues[0].GetType(), LUA_TSTRING);
            TEST_EQUAL(RetValues[0].Value<const char*>(), "Foo");
        });

        It(TEXT("调用失败，返回值被标记为无效"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            AddExpectedError(TEXT("Global table NotExistsTable doesn't exist!"));
            const UnLua::FLuaRetValues RetValues = UnLua::CallTableFunc(L, "NotExistsTable", "Func", "Foo");
            TEST_FALSE(RetValues.IsValid());
        });
    });

    Describe(TEXT("支持调用包含按引用传递参数的函数"), [this]
    {
        It(TEXT("蓝图：调用的函数本身没有返回值，依次返回参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            A = 10 -- int32
            B = 20 -- int32&
            C = 30 -- const int32&
            D = 'Test' -- FString&
            B, D = UE.UUnLuaTestFunctionLibrary.TestForBaseSpec1(A, B, C, D)
            )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "D");
            const auto D = lua_tostring(L, -1);
            TEST_EQUAL(D, "TestFlag");
        });

#if UNLUA_LEGACY_RETURN_ORDER
        It(TEXT("蓝图：调用的函数本身有返回值，先依次返回参数再返回返回值（UNLUA_LEGACY_RETURN_ORDER）"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
                    A = 10 -- int32
                    B = 20 -- int32&
                    C = 30 -- int32&
                    B, C, Ret = UE.UUnLuaTestFunctionLibrary.TestForBaseSpec2(A, B, C)
                    )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "Ret");
            const auto Ret = (bool)lua_toboolean(L, -1);
            TEST_TRUE(Ret);

            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "C");
            const auto C = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(C, 31);
        });
#else
        It(TEXT("蓝图：调用的函数本身有返回值，先返回返回值再依次返回参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            A = 10 -- int32
            B = 20 -- int32&
            C = 30 -- int32&
            Ret, B, C = UE.UUnLuaTestFunctionLibrary.TestForBaseSpec2(A, B, C)
            )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "Ret");
            const auto Ret = (bool)lua_toboolean(L, -1);
            TEST_TRUE(Ret);

            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "C");
            const auto C = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(C, 31);
        });
#endif

        It(TEXT("原生：调用的函数本身没有返回值，依次返回参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            A = 10 -- int32
            B = 20 -- int32&
            C = 30 -- const int32&
            D = 'Test' -- FString&
            B, D = UE.FUnLuaTestLib.TestForBaseSpec1(A, B, C, D)
            )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "D");
            const auto D = lua_tostring(L, -1);
            TEST_EQUAL(D, "TestFlag");
        });

#if UNLUA_LEGACY_RETURN_ORDER
        It(TEXT("原生：调用的函数本身有返回值，先依次返回参数再返回返回值（UNLUA_LEGACY_RETURN_ORDER）"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            A = 10 -- int32
            B = 20 -- int32&
            C = 30 -- int32&
            B, C, Ret = UE.FUnLuaTestLib.TestForBaseSpec2(A, B, C)
            )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "Ret");
            const auto Ret = (bool)lua_toboolean(L, -1);
            TEST_TRUE(Ret);

            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "C");
            const auto C = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(C, 31);
        });
#else
        It(TEXT("原生：调用的函数本身有返回值，先返回返回值再依次返回参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
            A = 10 -- int32
            B = 20 -- int32&
            C = 30 -- int32&
            Ret, B, C = UE.FUnLuaTestLib.TestForBaseSpec2(A, B, C)
            )";
            UnLua::RunChunk(L, Chunk);
            lua_getglobal(L, "Ret");
            const auto Ret = (bool)lua_toboolean(L, -1);
            TEST_TRUE(Ret);

            lua_getglobal(L, "B");
            const auto B = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(B, 21);

            lua_getglobal(L, "C");
            const auto C = (int32)lua_tointeger(L, -1);
            TEST_EQUAL(C, 31);
        });
#endif
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
