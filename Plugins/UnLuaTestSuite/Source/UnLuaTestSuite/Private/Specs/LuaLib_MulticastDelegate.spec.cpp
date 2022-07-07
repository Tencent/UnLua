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

BEGIN_DEFINE_SPEC(FUnLuaLibMulticastDelegateSpec, "UnLua.API.FMulticastScriptDelegate", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
    UUnLuaTestStub* Stub;
END_DEFINE_SPEC(FUnLuaLibMulticastDelegateSpec)

void FUnLuaLibMulticastDelegateSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
        Stub = NewObject<UUnLuaTestStub>();
        UnLua::PushUObject(L, Stub);
        lua_setglobal(L, "Stub");
    });

    Describe(TEXT("Add"), [this]()
    {
        It(TEXT("添加绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Flag = false
            Stub.SimpleEvent:Add(Stub, function() Flag = true end)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleEvent.IsBound());
            Stub->SimpleEvent.Broadcast();
            lua_getglobal(L, "Flag");
            TEST_TRUE(lua_toboolean(L, -1));
        });

        It(TEXT("添加绑定：赋值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Flag = false
            Stub.SimpleEvent = { Stub, function() Flag = true end }
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleEvent.IsBound());
            Stub->SimpleEvent.Broadcast();
            lua_getglobal(L, "Flag");
            TEST_TRUE(lua_toboolean(L, -1));
        });

        It(TEXT("添加绑定：UFunction"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleEvent:Add(Stub, Stub.AddCount)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleEvent.IsBound());
            Stub->SimpleEvent.Broadcast();
            TEST_EQUAL(Stub->Counter, 1);
        });
    });

    Describe(TEXT("Remove"), [this]()
    {
        It(TEXT("移除绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Callback = function() end
            Stub.SimpleEvent:Add(Stub, Callback)
            Stub.SimpleEvent:Remove(Stub, Callback)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(Stub->SimpleEvent.IsBound());
        });

        It(TEXT("移除绑定：UFunction"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleEvent:Add(Stub, Stub.AddCount)
            Stub.SimpleEvent:Remove(Stub, Stub.AddCount)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(Stub->SimpleEvent.IsBound());
        });
    });

    Describe(TEXT("Clear"), [this]()
    {
        It(TEXT("清理所有的绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleEvent:Add(Stub, function() end)
            Stub.SimpleEvent:Clear()
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(Stub->SimpleEvent.IsBound());
        });
    });

    Describe(TEXT("Broadcast"), [this]()
    {
        It(TEXT("广播事件"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Counter1 = 0
            local Counter2 = 0
            Stub.SimpleEvent:Add(Stub, function() Counter1 = Counter1 + 1 end)
            Stub.SimpleEvent:Add(Stub, function() Counter2 = Counter2 + 1 end)
            Stub.SimpleEvent:Broadcast()
            return Counter1, Counter2
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 1LL);
            TEST_EQUAL(lua_tointeger(L, -2), 1LL);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
