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

BEGIN_DEFINE_SPEC(FUnLuaLibDelegateSpec, "UnLua.API.FScriptDelegate", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
    UUnLuaTestStub* Stub;
END_DEFINE_SPEC(FUnLuaLibDelegateSpec)

void FUnLuaLibDelegateSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
        Stub = NewObject<UUnLuaTestStub>();
        UnLua::PushUObject(L, Stub);
        lua_setglobal(L, "Stub");
    });

    Describe(TEXT("Bind"), [this]()
    {
        It(TEXT("绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleHandler:Bind(Stub, function() end)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleHandler.IsBound());
        });
        It(TEXT("绑定：UFunction"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleHandler:Bind(Stub, Stub.AddCount)
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleHandler.IsBound());
            Stub->SimpleHandler.Execute();
            TEST_EQUAL(Stub->Counter, 1);
        });
        It(TEXT("绑定：赋值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            Stub.SimpleHandler = { Stub, function() end }
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(Stub->SimpleHandler.IsBound());
        });
    });

    Describe(TEXT("Unbind"), [this]()
    {
        It(TEXT("解绑"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Callback = function() end
            Stub.SimpleHandler:Bind(Stub, Callback)
            Stub.SimpleHandler:Unbind()
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(Stub->SimpleHandler.IsBound());
        });
    });

    Describe(TEXT("Execute"), [this]()
    {
        It(TEXT("执行委托"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Counter = 0
            Stub.SimpleHandler:Bind(Stub, function() Counter = Counter + 1 end)
            Stub.SimpleHandler:Execute()
            return Counter
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 1LL);
        });

        It(TEXT("执行委托：赋值"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Counter = 0
            Stub.SimpleHandler = {Stub, function() Counter = Counter + 1 end}
            Stub.SimpleHandler:Execute()
            return Counter
            )";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 1LL);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
