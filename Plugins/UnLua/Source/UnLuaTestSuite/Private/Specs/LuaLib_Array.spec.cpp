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
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

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
        It(TEXT("构造TArray<int32>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
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

        It(TEXT("构造TArray<FString>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray('')\
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

        It(TEXT("构造TArray<bool>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(true)\
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

        It(TEXT("构造TArray<FVector>"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(UE.FVector)\
            Array:Add(UE.FVector(1,2,3))\
            Array:Add(UE.FVector(3,2,1))\
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
        It(TEXT("获取数组长度"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
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
            local Array = UE.TArray(0)\
            Array:Add(1)\
            local Result = Array:Reserve(10)\
            return Result, Array\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -2));
        });
    });

    Describe(TEXT("Resize"), [this]
    {
        It(TEXT("扩容，使用默认元素填充"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Resize(3)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->Num(), 3);
            TEST_EQUAL(Array->operator[](0), 1);
            TEST_EQUAL(Array->operator[](1), 0);
            TEST_EQUAL(Array->operator[](2), 0);
        });

        It(TEXT("缩容，移除多余元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(1)\
            Array:Resize(1)\
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

    Describe(TEXT("GetData"), [this]
    {
        It(TEXT("获取数组分配的内存指针"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            return Array, Array:GetData()\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -2);
            TEST_TRUE(ScriptArray!=nullptr);
            TEST_EQUAL(lua_topointer(L, -1), ScriptArray->GetData());
        });
    });

    Describe(TEXT("Get"), [this]
    {
        It(TEXT("拷贝指定索引位置的元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(UE.FVector)\
            Array:Add(UE.FVector(1,1,1))\
            local Copied = Array:Get(1)\
            Copied:Set(2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<FVector>* Array = (TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), FVector(1,1,1));
        });
    });

    Describe(TEXT("GetRef"), [this]
    {
        It(TEXT("获取指定索引位置的元素引用"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(UE.FVector)\
            Array:Add(UE.FVector(1,1,1))\
            local Ref = Array:GetRef(1)\
            Ref:Set(2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<FVector>* Array = (TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), FVector(2,1,1));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置指定索引位置的元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(1)\
            Array:Set(1,2)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), 2);
        });
    });

    Describe(TEXT("Swap"), [this]
    {
        It(TEXT("交换两个索引位置的元素"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:Add(3)\
            Array:Swap(1,3)\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), 3);
            TEST_EQUAL(Array->operator[](2), 1);
        });
    });

    Describe(TEXT("Shuffle"), [this]
    {
        It(TEXT("随机打乱数组元素位置"), EAsyncExecution::ThreadPool, [this]()
        {
            FMath::RandInit(1);
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            Array:Add(3)\
            Array:Shuffle()\
            return Array\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), 1);
            TEST_EQUAL(Array->operator[](1), 3);
            TEST_EQUAL(Array->operator[](2), 2);
        });
    });

    Describe(TEXT("LastIndex"), [this]
    {
        It(TEXT("获取数组最后一个元素的索引位置"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            return Array:LastIndex()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    Describe(TEXT("IsValidIndex"), [this]
    {
        It(TEXT("合法位置索引返回true"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            return Array:IsValidIndex(1)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -1));
        });
        It(TEXT("非法位置索引返回false"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            return Array:IsValidIndex(0)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("Contains"), [this]
    {
        It(TEXT("数组包含指定元素返回true"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            return Array:Contains(1)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -1));
        });
        It(TEXT("数组不包含指定元素返回false"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            return Array:Contains(0)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("Append"), [this]
    {
        It(TEXT("追加另外一个数组的所有元素到数组末尾"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array1 = UE.TArray(0)\
            Array1:Add(1)\
            Array1:Add(2)\
            local Array2 = UE.TArray(0)\
            Array2:Add(3)\
            Array2:Add(4)\
            Array1:Append(Array2)\
            return Array1\
            ";
            UnLua::RunChunk(L, Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array->operator[](0), 1);
            TEST_EQUAL(Array->operator[](1), 2);
            TEST_EQUAL(Array->operator[](2), 3);
            TEST_EQUAL(Array->operator[](3), 4);
        });
    });

    Describe(TEXT("ToTable"), [this]
    {
        It(TEXT("将数组内容转为LuaTable"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Array = UE.TArray(0)\
            Array:Add(1)\
            Array:Add(2)\
            local Table = Array:ToTable()\
            return Table[1], Table[2]\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
            TEST_EQUAL(lua_tointeger(L, -2), 1LL);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
