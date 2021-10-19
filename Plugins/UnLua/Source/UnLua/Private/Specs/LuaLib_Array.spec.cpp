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

    Describe(TEXT("Length"), [this]
    {
        It(TEXT("正确获取长度"), EAsyncExecution::ThreadPool, [this]()
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

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("追加元素到数组末尾"), EAsyncExecution::ThreadPool, [this]()
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
    });

    Describe(TEXT("AddUnique"), [this]
    {
        It(TEXT("只追加不重复的元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:AddUnique(1)\
            Array:AddUnique(1)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 1);
            TEST_EQUAL(Array->operator[](0), 1);
        });
    });

    Describe(TEXT("Find"), [this]
    {
        It(TEXT("找到元素，返回索引位置"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array:Find(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });

        It(TEXT("没有找到元素，返回0"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array:Find(100)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 0LL);
        });
    });

    Describe(TEXT("Insert"), [this]
    {
        It(TEXT("插入元素到指定位置"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:Insert(3, 2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 3);
            TEST_EQUAL(Array->operator[](1), 3);
            TEST_EQUAL(Array->operator[](2), 2);
        });
    });

    Describe(TEXT("Remove"), [this]
    {
        It(TEXT("移除指定索引处的元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:Remove(2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 1);
            TEST_EQUAL(Array->operator[](0), 1);
        });
    });

    Describe(TEXT("RemoveItem"), [this]
    {
        It(TEXT("移除指定元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:RemoveItem(1)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 1);
            TEST_EQUAL(Array->operator[](0), 2);
        });
    });

    Describe(TEXT("Clear"), [this]
    {
        It(TEXT("清除数组，移除所有元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:Clear()\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 0);
        });
    });

    Describe(TEXT("Reserve"), [this]
    {
        It(TEXT("预留N个数组元素的空间"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            local Result = Array:Reserve(10)\
            return Result, Array\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -2));

            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->GetAllocatedSize(), (SIZE_T)10 * 4);
        });

        It(TEXT("只能够对空数组进行预留空间操作"), EAsyncExecution::ThreadPool, [this]()
        {
            AddExpectedError(TEXT("TArray_Reserve: 'Reserve' is only valid for empty TArray!"));
            const char* Chunk = "\
            local Array = UE4.TArray(0)\
            Array:Add(1)\
            local Result = Array:Reserve(10)\
            return Result, Array\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -2));
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
