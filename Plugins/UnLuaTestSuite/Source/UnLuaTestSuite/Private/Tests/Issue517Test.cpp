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


#include "Issue517Test.h"

#include "LowLevel.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue517 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto World = GetWorld();
        UnLua::PushUObject(L, World);
        lua_setglobal(L, "World");

         const auto Chunk1 = R"(
             Child = World:SpawnActor(UE.AIssue517Actor)
             StructRef = Child.Struct
             StructRef.X = 10
             StructRef.Name = "MyStruct"
             print(StructRef, StructRef.X, StructRef.Name)
             print(Child.Struct, Child.Struct.X, StructRef.Name)

             Child:K2_DestroyActor()
             )";
         UnLua::RunChunk(L, Chunk1);

         CollectGarbage(RF_Standalone, true);
         lua_getfield(L, LUA_REGISTRYINDEX, "StructMap");
         lua_pushnil(L);
         while (lua_next(L, -2) != 0)
         {
             bool TwoLevelPtr;
             auto Struct = (FIssue517Struct*)UnLua::GetPointer(L, -1, &TwoLevelPtr);
             if (Struct->X == 10)
             {
                 void* Userdata = GetUserdataFast(L, -1, &TwoLevelPtr);
                 check(TwoLevelPtr)
                 *((void**)Userdata) = (void*)UnLua::LowLevel::ReleasedPtr;
             }
             lua_pop(L, 1);
         }
         lua_pop(L, 1);
         
         const auto Chunk2 = R"(
             local registry = debug.getregistry()
             local map = registry.StructMap
             for k, v in pairs(map) do
                 print(k, v)
             end
             print(tostring(StructRef), StructRef.X, StructRef.Name)
             print(Child:GetName())
         )";
         UnLua::RunChunk(L, Chunk2);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue517, TEXT("UnLua.Regression.Issue517 Actor的Struct成员变量在Lua里引用，释放后仍旧可以访问"))

#endif
