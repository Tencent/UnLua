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

BEGIN_DEFINE_SPEC(FUnLuaLibMapSpec, "UnLua.API.TMap", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibMapSpec)

void FUnLuaLibMapSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("构造TMap"), [this]()
    {
        It(TEXT("构造TMap<int32,int32>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(2,2)\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<int32, int32>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 2);
            TEST_EQUAL(Map->operator[](1), 1);
            TEST_EQUAL(Map->operator[](2), 2);
        });

        It(TEXT("构造TMap<FString,FString>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap('','')\
            Map:Add('A','Apple')\
            Map:Add('B','Banana')\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<FString, FString>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 2);
            TEST_EQUAL(Map->operator[]("A"), "Apple");
            TEST_EQUAL(Map->operator[]("B"), "Banana");
        });

        It(TEXT("构造TMap<FVector,bool>"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(UE.FVector,true)\
            Map:Add(UE.FVector(1,1,1),true)\
            Map:Add(UE.FVector(2,2,2),false)\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<FVector, bool>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 2);
            TEST_EQUAL(Map->operator[](FVector(1,1,1)), true);
            TEST_EQUAL(Map->operator[](FVector(2,2,2)), false);
        });
    });

    Describe(TEXT("Length"), [this]()
    {
        It(TEXT("获取Map长度"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(2,2)\
            return Map:Length()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 2LL);
        });
    });

    Describe(TEXT("Add"), [this]()
    {
        It(TEXT("Key不存在，新增"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(2,2)\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<int32, int32>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 2);
            TEST_EQUAL(Map->operator[](1), 1);
            TEST_EQUAL(Map->operator[](2), 2);
        });

        It(TEXT("Key存在，覆盖"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(1,2)\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<int32, int32>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 1);
            TEST_EQUAL(Map->operator[](1), 2);
        });
    });

    Describe(TEXT("Remove"), [this]()
    {
        It(TEXT("Key不存在，返回false"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            return Map:Remove(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_FALSE(lua_toboolean(L, -1));
        });

        It(TEXT("Key存在，移除并返回true"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            return Map:Remove(1), Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<int32, int32>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->Num(), 0);
            TEST_TRUE(lua_toboolean(L, -2));
        });
    });

    Describe(TEXT("Find"), [this]()
    {
        It(TEXT("查找Key不存在，返回nil"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            return Map:Find(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_isnil(L, -1));
        });

        It(TEXT("查找Key存在，返回Value的拷贝"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            return Map:Find(1)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 1LL);
        });
    });

    Describe(TEXT("FindRef"), [this]()
    {
        It(TEXT("查找Key不存在，返回nil"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,UE.FVector)\
            Map:Add(1,UE.FVector(1,1,1))\
            return Map:FindRef(2)\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(lua_isnil(L, -1));
        });

        It(TEXT("查找Key存在，返回Value的引用"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,UE.FVector)\
            Map:Add(1,UE.FVector(1,1,1))\
            local Ref = Map:FindRef(1)\
            Ref:Set(2,2,2)\
            return Map\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Map = (TMap<int32, FVector>*)UnLua::GetMap(L, -1);
            TEST_EQUAL(Map->operator[](1), FVector(2,2,2));
        });
    });

    Describe(TEXT("Clear"), [this]()
    {
        It(TEXT("清除，移除所有键值对"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(2,2)\
            Map:Clear()\
            return Map:Length()\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 0LL);
        });
    });

    Describe(TEXT("Keys"), [this]()
    {
        It(TEXT("获取所有Key的数组"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,1)\
            Map:Add(2,2)\
            local Ret = UE.TArray(0)\
            Ret:Append(Map:Keys())\
            return Ret\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Array = (TArray<int32>*)UnLua::GetArray(L, -1);
            TEST_EQUAL(Array->Num(), 2);
            TEST_TRUE(Array->Contains(1));
            TEST_TRUE(Array->Contains(2));
        });
    });

    Describe(TEXT("Values"), [this]()
    {
        It(TEXT("获取所有Value的数组"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,3)\
            Map:Add(2,4)\
            local Ret = UE.TArray(0)\
            Ret:Append(Map:Values())\
            return Ret\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Array = (TArray<int32>*)UnLua::GetArray(L, -1);
            TEST_EQUAL(Array->Num(), 2);
            TEST_TRUE(Array->Contains(3));
            TEST_TRUE(Array->Contains(4));
        });
    });

    Describe(TEXT("ToTable"), [this]()
    {
        It(TEXT("将Map内容转为LuaTable"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Map = UE.TMap(0,0)\
            Map:Add(1,3)\
            Map:Add(2,4)\
            local Table = Map:ToTable()\
            return Table[1], Table[2]\
            ";
            UnLua::RunChunk(L, Chunk);
            TEST_EQUAL(lua_tointeger(L, -1), 4LL);
            TEST_EQUAL(lua_tointeger(L, -2), 3LL);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
