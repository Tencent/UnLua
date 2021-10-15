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

    UnLua::Push(L, static_cast<int8>(0x7F));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0x7FLL);

    UnLua::Push(L, static_cast<int16>(0x7FFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0x7FFFLL);

    UnLua::Push(L, static_cast<int32>(0x7FFFFFFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0x7FFFFFFFLL);

    UnLua::Push(L, static_cast<int64>(0x7FFFFFFFFFFFFFFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0x7FFFFFFFFFFFFFFFLL);

    UnLua::Push(L, static_cast<uint8>(0xFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0xFFLL);

    UnLua::Push(L, static_cast<uint16>(0xFFFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0xFFFFLL);

    UnLua::Push(L, static_cast<uint32>(0xFFFFFFFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0xFFFFFFFFLL);

    UnLua::Push(L, static_cast<uint64>(0xFFFFFFFFFFFFFFFF));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tointeger(L, -1), 0xFFFFFFFFFFFFFFFFLL);

    UnLua::Push(L, 0.123f);
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tonumber(L, -1), 0.123);

    UnLua::Push(L, 0.123);
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tonumber(L, -1), 0.123);

    UnLua::Push(L, true);
    TestEqual(TEXT("Return value on lua stack must correct"), lua_toboolean(L, -1), true);

    UnLua::Push(L, "A");
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tostring(L, -1), "A");

    UnLua::Push(L, FString("Hello"));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tostring(L, -1), "Hello");

    UnLua::Push(L, FString("虚幻引擎"));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tostring(L, -1), "虚幻引擎");

    UnLua::Push(L, FName("Foo"));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tostring(L, -1), "Foo");

    UnLua::Push(L, static_cast<void*>(L));
    TestEqual(TEXT("Return value on lua stack must correct"), reinterpret_cast<SIZE_T>(lua_topointer(L, -1)), reinterpret_cast<SIZE_T>(L));

    FVector VectorValue(1, 2, 3);
    UnLua::Push<FVector>(L, &VectorValue);
    TestEqual(TEXT("Return value on lua stack must correct"), UnLua::Get(L, -1, UnLua::TType<FVector>()), VectorValue);

    lua_settop(L, 1);

    TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, "return 123"));
    TestEqual(TEXT("Return value on lua stack must correct"), lua_tonumber(L, -1), 123.0);

    UnLua::Shutdown();

    TestNull(TEXT("Lua state must closed"), UnLua::GetState());

    return true;
}


#endif //WITH_DEV_AUTOMATION_TESTS
