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

BEGIN_DEFINE_SPEC(FUnLuaLibSetSpec, "UnLua.API.TSet", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibSetSpec)

void FUnLuaLibSetSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造TSet"), [this]()
    {
        It(TEXT("构造TSet<int32>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            return Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<int32>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 2);
            TEST_TRUE(Set->Contains(1));
            TEST_TRUE(Set->Contains(2));
        });

        It(TEXT("构造TSet<FString>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet('')\
            Set:Add('A')\
            Set:Add('B')\
            return Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<FString>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 2);
            TEST_TRUE(Set->Contains("A"));
            TEST_TRUE(Set->Contains("B"));
        });

        It(TEXT("构造TSet<FVector>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(UE.FVector)\
            Set:Add(UE.FVector(1,1,1))\
            Set:Add(UE.FVector(2,2,2))\
            return Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<FVector>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 2);
            TEST_TRUE(Set->Contains(FVector(1,1,1)));
            TEST_TRUE(Set->Contains(FVector(2,2,2)));
        });

        It(TEXT("构造TSet<UScriptStruct>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = R"(
            local Struct_TableRow = UE.UObject.Load("/UnLuaTestSuite/Tests/Misc/Struct_TableRow.Struct_TableRow")
            local Set = UE.TSet(Struct_TableRow)

            local Item1 = Struct_TableRow()
            Item1.TestFloat = 1
            local Item2 = Struct_TableRow()
            Item2.TestFloat = 2

            Set:Add(Item1)
            Set:Add(Item2)

            return Set
            )";
            UnLua::RunChunk(L, Chunk);
            const auto Set = UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 2);
        });
    });

    Describe(TEXT("Length"), [this]()
    {
        It(TEXT("获取Set长度"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            return Set:Length()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    Describe(TEXT("Add"), [this]()
    {
        It(TEXT("元素不存在，新增"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            return Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<int32>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 2);
            TEST_TRUE(Set->Contains(1));
            TEST_TRUE(Set->Contains(2));
        });

        It(TEXT("不增加重复的元素"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(1)\
            return Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<int32>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 1);
            TEST_TRUE(Set->Contains(1));
        });
    });

    Describe(TEXT("Remove"), [this]()
    {
        It(TEXT("元素不存在，返回false"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            return Set:Remove(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });

        It(TEXT("元素存在，移除并返回true"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            return Set:Remove(1), Set\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Set = (TSet<int32>*)UnLua::GetSet(L, -1);
            TEST_EQUAL(Set->Num(), 0);
            TEST_TRUE(lua_toboolean(L, -2));
        });
    });

    Describe(TEXT("Contains"), [this]()
    {
        It(TEXT("查找元素存在，返回true"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            return Set:Contains(1)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_toboolean(L, -1));
        });

        It(TEXT("查找元素不存在，返回false"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            return Set:Contains(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("Clear"), [this]()
    {
        It(TEXT("清除，移除所有元素"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            Set:Clear()\
            return Set:Length()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 0LL);
        });
    });

    Describe(TEXT("ToArray"), [this]()
    {
        It(TEXT("获取所有元素的数组"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            local Ret = UE.TArray(0)\
            Ret:Append(Set:ToArray())\
            return Ret\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Array = (TArray<int32>*)UnLua::GetArray(L, -1);
            TEST_EQUAL(Array->Num(), 2);
            TEST_TRUE(Array->Contains(1));
            TEST_TRUE(Array->Contains(2));
        });
    });

    Describe(TEXT("ToTable"), [this]()
    {
        It(TEXT("将Set内容转为LuaTable"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Set = UE.TSet(0)\
            Set:Add(1)\
            Set:Add(2)\
            local Table = Set:ToTable()\
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
