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

#include "UnLuaTestCommon.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue299 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual FString GetMapName() override
    {
        return "/Game/Tests/Regression/Issue299/Issue299_1";
    }
    
    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto World = GetWorld();
        UnLua::PushUObject(L, World);
        lua_setglobal(L, "G_World");

        const auto Chunk = "\
        local ActorClass = UE.UClass.Load('/Game/Tests/Regression/Issue299/BP_UnLuaTestActor_Issue299.BP_UnLuaTestActor_Issue299_C')\
        local TestActor = G_World:SpawnActor(ActorClass)\
        return TestActor\
        \
        ";

        UnLua::RunChunk(L, Chunk);

        const auto TestActor = (AActor*)UnLua::GetUObject(L, -1);
        const auto TargetFunc = TestActor->FindFunction(TEXT("TestForIssue299Copy"));

        lua_gc(L, LUA_GCCOLLECT, 0);
        CollectGarbage(RF_NoFlags, true);

        LoadMap("/Game/Tests/Regression/Issue299/Issue299_2");

        lua_gc(L, LUA_GCCOLLECT, 0);
        CollectGarbage(RF_NoFlags, true);
        
        GetTestRunner().AddExpectedError(TEXT("slot"));
        const auto bIsValid = TargetFunc->IsValidLowLevel();
        RUNNER_TEST_FALSE(bIsValid);

        const auto LastExists = GReflectionRegistry.UnRegisterFunction(TargetFunc);
        RUNNER_TEST_FALSE(LastExists);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue299, TEXT("UnLua.Regression.Issue299 真机环境覆写UFunction指针被重用导致的崩溃"))

#endif //WITH_DEV_AUTOMATION_TESTS
