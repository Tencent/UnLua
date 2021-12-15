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
END_DEFINE_SPEC(FUnLuaLibDelegateSpec)

void FUnLuaLibDelegateSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("Bind"), [this]()
    {
        It(TEXT("绑定"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            Stub.SimpleHandler:Bind(Stub, function() end)\
            return Stub\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Stub = (UUnLuaTestStub*)UnLua::GetUObject(L, -1);
            TEST_TRUE(Stub->SimpleHandler.IsBound());
        });
    });

    Describe(TEXT("Unbind"), [this]()
    {
        It(TEXT("解绑"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            local Callback = function() end\
            Stub.SimpleHandler:Bind(Stub, Callback)\
            Stub.SimpleHandler:Unbind()\
            return Stub\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Stub = (UUnLuaTestStub*)UnLua::GetUObject(L, -1);
            TEST_FALSE(Stub->SimpleHandler.IsBound());
        });
    });

    Describe(TEXT("Execute"), [this]()
    {
        It(TEXT("执行委托"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Stub = NewObject(UE.UUnLuaTestStub)\
            local Counter = 0\
            Stub.SimpleHandler:Bind(Stub, function() Counter = Counter + 1 end)\
            Stub.SimpleHandler:Execute()\
            return Counter\
            ";
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
