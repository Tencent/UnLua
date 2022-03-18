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
END_DEFINE_SPEC(FUnLuaLibMulticastDelegateSpec)

void FUnLuaLibMulticastDelegateSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("Add"), [this]()
    {
        It(TEXT("添加绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            Stub.SimpleEvent:Add(Stub, function() end)\
            return Stub\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Stub = (UUnLuaTestStub*)UnLua::GetUObject(L, -1);
            TEST_TRUE(Stub->SimpleEvent.IsBound());
        });
    });

    Describe(TEXT("Remove"), [this]()
    {
        It(TEXT("移除绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            local Callback = function() end\
            Stub.SimpleEvent:Add(Stub, Callback)\
            Stub.SimpleEvent:Remove(Stub, Callback)\
            return Stub\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Stub = (UUnLuaTestStub*)UnLua::GetUObject(L, -1);
            TEST_FALSE(Stub->SimpleEvent.IsBound());
        });
    });

    Describe(TEXT("Clear"), [this]()
    {
        It(TEXT("清理所有的绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            Stub.SimpleEvent:Add(Stub, function() end)\
            Stub.SimpleEvent:Clear()\
            return Stub\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Stub = (UUnLuaTestStub*)UnLua::GetUObject(L, -1);
            TEST_FALSE(Stub->SimpleEvent.IsBound());
        });
    });

    Describe(TEXT("Broadcast"), [this]()
    {
        It(TEXT("广播事件"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            local Counter1 = 0\
            local Counter2 = 0\
            Stub.SimpleEvent:Add(Stub, function() Counter1 = Counter1 + 1 end)\
            Stub.SimpleEvent:Add(Stub, function() Counter2 = Counter2 + 1 end)\
            Stub.SimpleEvent:Broadcast()\
            return Counter1, Counter2\
            ";
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
