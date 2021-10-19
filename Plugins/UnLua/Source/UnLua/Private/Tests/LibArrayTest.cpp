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

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Misc/AutomationTest.h"
#include "UnLua.h"
#include "Containers/LuaArray.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibArrayTest, "UnLua.LuaLib_Array", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)


bool FLibArrayTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("UnLua Startup must succeed"), UnLua::Startup());
    lua_State* L = UnLua::GetState();
    TestNotNull(TEXT("Lua state must created"), L);

    {
        const char* Chunk = "\
        local Array = UE4.TArray(0)\
        Array:Add(1)\
        Array:Add(2)\
        return Array\
        ";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
        TestNotNull(TEXT("Return value must correct"), ScriptArray);

        const TArray<int32>* Array = (TArray<int32>*)ScriptArray;
        TestNotNull(TEXT("Return value must correct"), Array);
        TestEqual(TEXT("Size of array must correct"), Array->Num(), 2);
        TestEqual(TEXT("Content of Array[0] must correct"), Array->operator[](0), 1);
        TestEqual(TEXT("Content of Array[1] must correct"), Array->operator[](1), 2);
    }

    {
        const char* Chunk = "\
        local Array = UE4.TArray('')\
        Array:Add('A')\
        Array:Add('B')\
        return Array\
        ";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
        TestNotNull(TEXT("Return value must correct"), ScriptArray);

        const TArray<FString>* Array = (TArray<FString>*)ScriptArray;
        TestNotNull(TEXT("Return value must correct"), Array);
        TestEqual(TEXT("Size of array must correct"), Array->Num(), 2);
        TestEqual(TEXT("Content of Array[0] must correct"), Array->operator[](0), "A");
        TestEqual(TEXT("Content of Array[1] must correct"), Array->operator[](1), "B");
    }

    {
        const char* Chunk = "\
        local Array = UE4.TArray(true)\
        Array:Add(true)\
        Array:Add(false)\
        return Array\
        ";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
        TestNotNull(TEXT("Return value must correct"), ScriptArray);

        const TArray<bool>* Array = (TArray<bool>*)ScriptArray;
        TestNotNull(TEXT("Return value must correct"), Array);
        TestEqual(TEXT("Size of array must correct"), Array->Num(), 2);
        TestEqual(TEXT("Content of Array[0] must correct"), Array->operator[](0), true);
        TestEqual(TEXT("Content of Array[1] must correct"), Array->operator[](1), false);
    }

    {
        const char* Chunk = "\
        local Array = UE4.TArray(UE4.FVector)\
        Array:Add(UE4.FVector(1,2,3))\
        Array:Add(UE4.FVector(3,2,1))\
        return Array\
        ";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        const FScriptArray* ScriptArray = UnLua::GetArray(L, -1);
        TestNotNull(TEXT("Return value must correct"), ScriptArray);

        const TArray<FVector>* Array = (TArray<FVector>*)ScriptArray;
        TestNotNull(TEXT("Return value must correct"), Array);
        TestEqual(TEXT("Size of array must correct"), Array->Num(), 2);
        TestEqual(TEXT("Content of Array[0] must correct"), Array->operator[](0), FVector(1, 2, 3));
        TestEqual(TEXT("Content of Array[1] must correct"), Array->operator[](1), FVector(3, 2, 1));
    }

    {
        const char* Chunk = "\
        local Array = UE4.TArray(0)\
        Array:Add(1)\
        Array:Add(2)\
        return Array:Length()\
        ";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        TestEqual(TEXT("Length of array must correct"), lua_tointeger(L, -1), 2LL);
    }

    UnLua::Shutdown();

    TestNull(TEXT("Lua state must closed"), UnLua::GetState());

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
