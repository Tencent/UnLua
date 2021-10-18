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

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Misc/Timespan.h"
#include "Misc/AutomationTest.h"
#include "UnLua.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPITest, "UnLua.API", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)


bool FAPITest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("UnLua Startup must succeed"), UnLua::Startup());

    lua_State* L = UnLua::GetState();

    TestNotNull(TEXT("Lua state must created"), L);

    {
        constexpr auto TypeMsg = TEXT("Type of return value on lua stack must correct");
        constexpr auto ValueMsg = TEXT("Return value on lua stack must correct");
        UnLua::Push(L, static_cast<int8>(0x7F));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<int8>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0x7FLL);

        UnLua::Push(L, static_cast<int16>(0x7FFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<int16>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0x7FFFLL);

        UnLua::Push(L, static_cast<int32>(0x7FFFFFFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<int32>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0x7FFFFFFFLL);

        UnLua::Push(L, static_cast<int64>(0x7FFFFFFFFFFFFFFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<int64>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0x7FFFFFFFFFFFFFFFLL);

        UnLua::Push(L, static_cast<uint8>(0xFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<uint8>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0xFFLL);

        UnLua::Push(L, static_cast<uint16>(0xFFFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<uint16>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0xFFFFLL);

        UnLua::Push(L, static_cast<uint32>(0xFFFFFFFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<uint32>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0xFFFFFFFFLL);

        UnLua::Push(L, static_cast<uint64>(0xFFFFFFFFFFFFFFFF));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<uint64>()));
        TestEqual(ValueMsg, lua_tointeger(L, -1), 0xFFFFFFFFFFFFFFFFLL);

        UnLua::Push(L, 0.123f);
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<float>()));
        TestEqual(ValueMsg, lua_tonumber(L, -1), 0.123);

        UnLua::Push(L, 0.123);
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<double>()));
        TestEqual(ValueMsg, lua_tonumber(L, -1), 0.123);

        UnLua::Push(L, true);
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<bool>()));
        TestEqual(ValueMsg, lua_toboolean(L, -1), true);

        UnLua::Push(L, "A");
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<const char*>()));
        TestEqual(ValueMsg, lua_tostring(L, -1), "A");

        UnLua::Push(L, FString("Hello"));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<FString>()));
        TestEqual(ValueMsg, lua_tostring(L, -1), "Hello");

        UnLua::Push(L, FString("虚幻引擎"));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<FString>()));
        TestEqual(ValueMsg, lua_tostring(L, -1), "虚幻引擎");

        UnLua::Push(L, FName("Foo"));
        TestTrue(TypeMsg, UnLua::IsType(L, -1, UnLua::TType<FName>()));
        TestEqual(ValueMsg, lua_tostring(L, -1), "Foo");

        UnLua::Push(L, static_cast<void*>(L));
        TestEqual(ValueMsg, reinterpret_cast<SIZE_T>(lua_topointer(L, -1)), reinterpret_cast<SIZE_T>(L));

        FVector VectorValue(1, 2, 3);
        UnLua::Push<FVector>(L, &VectorValue);
        TestEqual(ValueMsg, UnLua::Get(L, -1, UnLua::TType<FVector>()), VectorValue);

        lua_settop(L, 1);
    }

    {
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, "return 123"));
        TestEqual(TEXT("Return value on lua stack must correct"), lua_tonumber(L, -1), 123.0);
        lua_settop(L, 1);
    }

    {
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, "function GlobalFunction(v) return v end"));
        const UnLua::FLuaRetValues RetValues = UnLua::Call(L, "GlobalFunction", "Foo");
        TestEqual(TEXT("Return value count must correct"), RetValues.Num(), 1);
        TestEqual(TEXT("Return type must correct"), RetValues[0].GetType(), LUA_TSTRING);
        TestEqual(TEXT("Return value must correct"), RetValues[0].Value<const char*>(), "Foo");
        lua_settop(L, 1);
    }

    {
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, "GlobalTable = {}; function GlobalTable.Func(v) return v end"));
        const UnLua::FLuaRetValues RetValues = UnLua::CallTableFunc(L, "GlobalTable", "Func", "Foo");
        TestEqual(TEXT("Return value count must correct"), RetValues.Num(), 1);
        TestEqual(TEXT("Return type must correct"), RetValues[0].GetType(), LUA_TSTRING);
        TestEqual(TEXT("Return value must correct"), RetValues[0].Value<const char*>(), "Foo");
        lua_settop(L, 1);
    }

    UnLua::Shutdown();

    TestNull(TEXT("Lua state must closed"), UnLua::GetState());

    return true;
}


#endif //WITH_DEV_AUTOMATION_TESTS
