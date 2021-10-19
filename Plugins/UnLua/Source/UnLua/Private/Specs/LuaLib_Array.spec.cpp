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

BEGIN_DEFINE_SPEC(FUnLuaLibArraySpec, "UnLua.API.TArray", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibArraySpec)

void FUnLuaLibArraySpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("构造TArray"), [this]()
    {
        It(TEXT("正确构造TArray<int32>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 2);
            TEST_EQUAL(Array->operator[](0), 1);
            TEST_EQUAL(Array->operator[](1), 2);
        });

        It(TEXT("正确构造TArray<FString>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray('')\
            Array:Add('A')\
            Array:Add('B')\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<FString>* Array = (TArray<FString>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 2);
            TEST_EQUAL(Array->operator[](0), "A");
            TEST_EQUAL(Array->operator[](1), "B");
        });

        It(TEXT("正确构造TArray<bool>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(true)\
            Array:Add(true)\
            Array:Add(false)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<bool>* Array = (TArray<bool>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 2);
            TEST_EQUAL(Array->operator[](0), true);
            TEST_EQUAL(Array->operator[](1), false);
        });

        It(TEXT("正确构造TArray<FVector>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(UE4.FVector)\
            Array:Add(UE4.FVector(1,2,3))\
            Array:Add(UE4.FVector(3,2,1))\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<FVector>* Array = (TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 2);
            TEST_EQUAL(Array->operator[](0), FVector(1, 2, 3));
            TEST_EQUAL(Array->operator[](1), FVector(3, 2, 1));
        });
    });

    Describe(TEXT("获取TArray长度"), [this]
    {
        It(TEXT("Length"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array:Length()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });

        xIt(TEXT("Num"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array:Num()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
