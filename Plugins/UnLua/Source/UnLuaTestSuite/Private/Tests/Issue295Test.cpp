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
#include "Components/CapsuleComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue295 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual FString GetMapName() override
    {
        return "/Game/Tests/Regression/Issue295/Issue295_1";
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        UnLua::PushUObject(L, GetWorld());
        lua_setglobal(L, "G_World");

        const char* Chunk1 = "\
            local UMGClass = UE.UClass.Load('/Game/Tests/Regression/Issue295/UnLuaTestUMG_Issue295.UnLuaTestUMG_Issue295_C') \
            local Outer = G_World\
            \
            local UMG1 = NewObject(UMGClass, Outer, 'UMG1', 'Tests.Regression.Issue295.TestUMG')\
            UMG1:AddToViewport()\
            \
            local UMG2 = NewObject(UMGClass, Outer, 'UMG2', 'Tests.Regression.Issue295.TestUMG')\
            UMG2:AddToViewport()\
            ";
        UnLua::RunChunk(L, Chunk1);

        lua_gc(L, LUA_GCCOLLECT, 0);
        CollectGarbage(RF_NoFlags, true);

        LoadMap("/Game/Tests/Regression/Issue295/Issue295_2");

        UnLua::PushUObject(L, GetWorld());
        lua_setglobal(L, "G_World");

        const char* Chunk2 = "\
            local UMGClass = UE.UClass.Load('/Game/Tests/Regression/Issue295/UnLuaTestUMG_Issue295.UnLuaTestUMG_Issue295_C') \
            local Outer = G_World\
            \
            local UMG1 = NewObject(UMGClass, Outer, 'UMG1', 'Tests.Regression.Issue295.TestUMG')\
            UMG1:AddToViewport()\
            \
            local UMG2 = NewObject(UMGClass, Outer, 'UMG2', 'Tests.Regression.Issue295.TestUMG')\
            UMG2:AddToViewport()\
            ";
        UnLua::RunChunk(L, Chunk2);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue295, TEXT("UnLua.Regression.Issue295 动态绑定的UMG对象，在切换地图回调Destruct导致崩溃"))

#endif //WITH_DEV_AUTOMATION_TESTS
