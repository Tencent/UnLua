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

#include "Tests/Issue398Test.h"
#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue398 : FUnLuaTestBase
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
        lua_setglobal(L, "G_World");
        const auto Chunk = R"(
            local CharacterClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue398/BP_Character.BP_Character_C")
            local Character = G_World:SpawnActor(CharacterClass)
            local Array = UE.TArray(UE.AActor)
            Array:Add(Character)
            Array:Add(Character)
            Array:Add(Character)
            Array:Add(Character)
            print("Array=", Array)
            local FunctionLibrary = UE.UClass.Load('/UnLuaTestSuite/Tests/Regression/Issue398/BP_FunctionLibrary.BP_FunctionLibrary_C')
            local ResultArray = FunctionLibrary.Test(Array)
            print("ResultArray=", ResultArray)
            Result = 0
            for i=1, 4 do
                print(ResultArray:Get(i))
                if ResultArray:Get(i) ~= nil then
                    Result = Result + 1
                end
            end 
        )";
        UnLua::RunChunk(L, Chunk);
        lua_getglobal(L, "Result");
        const auto Result = (int)lua_tointeger(L, -1);
        RUNNER_TEST_EQUAL(Result, 4);
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue398, TEXT("UnLua.Regression.Issue398 接口列表从lua传到c++/蓝图层会丢失一半信息"))

#endif //WITH_DEV_AUTOMATION_TESTS
