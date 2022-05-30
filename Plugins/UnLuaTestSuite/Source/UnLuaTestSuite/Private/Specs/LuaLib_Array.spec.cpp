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
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibArraySpec)

void FUnLuaLibArraySpec::Define()
{
    BeforeEach([this]
    {
        Env = MakeShared<UnLua::FLuaEnv>();
        L = Env->GetMainState();
    });

    AfterEach([this]
    {
        Env.Reset();
        L = nullptr;
    });

    Describe(TEXT("构造TArray"), [this]
    {
        It(TEXT("构造TArray<int32>"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array
            )";
            Env->DoString(Chunk);
            const auto ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
            TEST_EQUAL(Array[0], 1);
            TEST_EQUAL(Array[1], 2);
        });

        It(TEXT("构造TArray<FString>"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray('')
            Array:Add('A')
            Array:Add('B')
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FString>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
            TEST_EQUAL(Array[0], "A");
            TEST_EQUAL(Array[1], "B");
        });

        It(TEXT("构造TArray<bool>"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(true)
            Array:Add(true)
            Array:Add(false)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<bool>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
            TEST_EQUAL(Array[0], true);
            TEST_EQUAL(Array[1], false);
        });

        It(TEXT("构造TArray<FVector>"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(UE.FVector)
            Array:Add(UE.FVector(1,2,3))
            Array:Add(UE.FVector(3,2,1))
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
            TEST_EQUAL(Array[0], FVector(1, 2, 3));
            TEST_EQUAL(Array[1], FVector(3, 2, 1));
        });

        It(TEXT("构造TArray<UStruct>"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Struct_TableRow = UE.UObject.Load("/UnLuaTestSuite/Tests/Misc/Struct_TableRow.Struct_TableRow")
            local Array = UE.TArray(Struct_TableRow)

            local Item1 = Struct_TableRow()
            local Item2 = Struct_TableRow()

            Array:Add(Item1)
            Array:Add(Item2)

            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
        });
    });

    Describe(TEXT("Length"), [this]
    {
        It(TEXT("获取数组长度"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array:Length()
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });

        It(TEXT("Num"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array:Num()
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("追加元素到数组末尾"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 2);
            TEST_EQUAL(Array[0], 1);
            TEST_EQUAL(Array[1], 2);
        });
    });

    Describe(TEXT("AddUnique"), [this]
    {
        It(TEXT("只追加不重复的元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:AddUnique(1)
            Array:AddUnique(1)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 1);
            TEST_EQUAL(Array[0], 1);
        });
    });

    Describe(TEXT("Find"), [this]
    {
        It(TEXT("找到元素，返回索引位置"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array:Find(2)
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });

        It(TEXT("没有找到元素，返回0"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array:Find(100)
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 0LL);
        });
    });

    Describe(TEXT("Insert"), [this]
    {
        It(TEXT("插入元素到指定位置"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:Insert(3, 2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 3);
            TEST_EQUAL(Array[1], 3);
            TEST_EQUAL(Array[2], 2);
        });
    });

    Describe(TEXT("Remove"), [this]
    {
        It(TEXT("移除指定索引处的元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:Remove(2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 1);
            TEST_EQUAL(Array[0], 1);
        });
    });

    Describe(TEXT("RemoveItem"), [this]
    {
        It(TEXT("移除指定元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:RemoveItem(1)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 1);
            TEST_EQUAL(Array[0], 2);
        });
    });

    Describe(TEXT("Clear"), [this]
    {
        It(TEXT("清除数组，移除所有元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:Clear()
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 0);
        });
    });

    Describe(TEXT("Reserve"), [this]
    {
        It(TEXT("预留N个数组元素的空间"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            local Result = Array:Reserve(10)
            return Result, Array
            )";
            Env->DoString(Chunk);
            TEST_TRUE(lua_toboolean(L, -2));

            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.GetAllocatedSize(), (SIZE_T)10 * 4);
        });

        It(TEXT("只能够对空数组进行预留空间操作"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            AddExpectedError(TEXT("TArray_Reserve: 'Reserve' is only valid for empty TArray!"));
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            local Result = Array:Reserve(10)
            return Result, Array
            )";
            Env->DoString(Chunk);
            TEST_FALSE(lua_toboolean(L, -2));
        });
    });

    Describe(TEXT("Resize"), [this]
    {
        It(TEXT("扩容，使用默认元素填充"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Resize(3)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 3);
            TEST_EQUAL(Array[0], 1);
            TEST_EQUAL(Array[1], 0);
            TEST_EQUAL(Array[2], 0);
        });

        It(TEXT("缩容，移除多余元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(1)
            Array:Resize(1)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array.Num(), 1);
            TEST_EQUAL(Array[0], 1);
        });
    });

    Describe(TEXT("GetData"), [this]
    {
        It(TEXT("获取数组分配的内存指针"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            return Array, Array:GetData()
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -2);
            TEST_TRUE(ScriptArray!=nullptr);
            TEST_EQUAL(lua_topointer(L, -1), ScriptArray->GetData());
        });
    });

    Describe(TEXT("Get"), [this]
    {
        It(TEXT("拷贝指定索引位置的元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(UE.FVector)
            Array:Add(UE.FVector(1,1,1))
            local Copied = Array:Get(1)
            Copied:Set(2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array[0], FVector(1,1,1));
        });

        It(TEXT("使用[]获取"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(UE.FVector)
            Array:Add(UE.FVector(1,1,1))
            local Copied = Array[1]
            Copied:Set(2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array[0], FVector(1,1,1));
        });
    });

    Describe(TEXT("GetRef"), [this]
    {
        It(TEXT("获取指定索引位置的元素引用"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(UE.FVector)
            Array:Add(UE.FVector(1,1,1))
            local Ref = Array:GetRef(1)
            Ref:Set(2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<FVector>*)ScriptArray;
            TEST_EQUAL(Array[0], FVector(2,1,1));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置指定索引位置的元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(1)
            Array:Set(1,2)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array[0], 2);
        });

        It(TEXT("使用[]赋值"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(1)
            Array[1] = 2
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);

            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array[0], 2);
        });
    });

    Describe(TEXT("Swap"), [this]
    {
        It(TEXT("交换两个索引位置的元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:Add(3)
            Array:Swap(1,3)
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array[0], 3);
            TEST_EQUAL(Array[2], 1);
        });
    });

    Describe(TEXT("Shuffle"), [this]
    {
        It(TEXT("随机打乱数组元素位置"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            FMath::RandInit(1);
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            Array:Add(3)
            Array:Shuffle()
            return Array
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array[0], 1);
            TEST_EQUAL(Array[1], 3);
            TEST_EQUAL(Array[2], 2);
        });
    });

    Describe(TEXT("LastIndex"), [this]
    {
        It(TEXT("获取数组最后一个元素的索引位置"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            return Array:LastIndex()
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    Describe(TEXT("IsValidIndex"), [this]
    {
        It(TEXT("合法位置索引返回true"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            return Array:IsValidIndex(1)
            )";
            Env->DoString(Chunk);
            TEST_TRUE(lua_toboolean(L, -1));
        });
        It(TEXT("非法位置索引返回false"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            return Array:IsValidIndex(0)
            )";
            Env->DoString(Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("Contains"), [this]
    {
        It(TEXT("数组包含指定元素返回true"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            return Array:Contains(1)
            )";
            Env->DoString(Chunk);
            TEST_TRUE(lua_toboolean(L, -1));
        });
        It(TEXT("数组不包含指定元素返回false"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            return Array:Contains(0)
            )";
            Env->DoString(Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("Append"), [this]
    {
        It(TEXT("追加另外一个数组的所有元素到数组末尾"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array1 = UE.TArray(0)
            Array1:Add(1)
            Array1:Add(2)
            local Array2 = UE.TArray(0)
            Array2:Add(3)
            Array2:Add(4)
            Array1:Append(Array2)
            return Array1
            )";
            Env->DoString(Chunk);
            const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
            TEST_TRUE(ScriptArray!=nullptr);
            const auto& Array = *(TArray<int32>*)ScriptArray;
            TEST_EQUAL(Array[0], 1);
            TEST_EQUAL(Array[1], 2);
            TEST_EQUAL(Array[2], 3);
            TEST_EQUAL(Array[3], 4);
        });
    });

    Describe(TEXT("ToTable"), [this]
    {
        It(TEXT("将数组内容转为LuaTable"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(1)
            Array:Add(2)
            local Table = Array:ToTable()
            return Table[1], Table[2]
            )";
            Env->DoString(Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
            TEST_EQUAL(lua_tointeger(L, -2), 1LL);
        });
    });

    Describe(TEXT("pairs"), [this]
    {
        It(TEXT("迭代获取数组索引与元素"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Array = UE.TArray(0)
            Array:Add(100)
            Array:Add(200)

            local ret = {}
            for i, v in pairs(Array) do
                table.insert(ret, i)
                table.insert(ret, v)
            end
            return ret;
            )";
            TEST_TRUE(Env->DoString(Chunk));

            const auto Ret = UnLua::FLuaTable(Env.Get(), -1);
            TEST_EQUAL(Ret.Length(), 4);
            TEST_EQUAL(Ret[1].Value<int>(), 1)
            TEST_EQUAL(Ret[2].Value<int>(), 100)
            TEST_EQUAL(Ret[3].Value<int>(), 2)
            TEST_EQUAL(Ret[4].Value<int>(), 200)
        });

        It(TEXT("按引用获取元素（原生）"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local UUnLuaTestStub = UE.UUnLuaTestStub
            local Array = UE.TArray(UE.UUnLuaTestStub)

            local Stub1 = NewObject(UUnLuaTestStub)
            local Stub2 = NewObject(UUnLuaTestStub)

            Array:Add(Stub1)
            Array:Add(Stub2)

            for i, v in pairs(Array) do
                v.Counter = i
            end

            return Array:Get(1).Counter, Array:Get(2).Counter  
            )";
            TEST_TRUE(Env->DoString(Chunk));

            const auto Result1 = (int)lua_tointeger(L, -2);
            const auto Result2 = (int)lua_tointeger(L, -1);

            TEST_EQUAL(Result1, 1);
            TEST_EQUAL(Result2, 2);
        });

        It(TEXT("按引用获取元素（蓝图）"), EAsyncExecution::TaskGraphMainThread, [this]
        {
            const auto Chunk = R"(
            local Struct_TableRow = UE.UObject.Load("/UnLuaTestSuite/Tests/Misc/Struct_TableRow.Struct_TableRow")
            local Array = UE.TArray(Struct_TableRow)

            local Row1 = Struct_TableRow()
            local Row2 = Struct_TableRow()

            Array:Add(Row1)
            Array:Add(Row2)

            for i, v in pairs(Array) do
                v.TestFloat = i
            end

            return Array:Get(1).TestFloat, Array:Get(2).TestFloat  
            )";
            TEST_TRUE(Env->DoString(Chunk));

            const auto Result1 = (int)lua_tointeger(L, -2);
            const auto Result2 = (int)lua_tointeger(L, -1);

            TEST_EQUAL(Result1, 1);
            TEST_EQUAL(Result2, 2);
        });
    });
}

#endif
