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
#include "GameFramework/GameModeBase.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibClassSpec, "UnLua.API.UClass", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibClassSpec)

void FUnLuaLibClassSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("Load"), [this]()
    {
        It(TEXT("正确加载蓝图类"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const UClass* Expected = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Core/Blueprints/BP_Game.BP_Game_C"));
            UnLua::RunChunk(L, "return UE.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')");
            const UClass* Actual = static_cast<UClass*>(UnLua::GetPointer(L, -1));
            TEST_EQUAL(Actual, Expected);
        });
    });

    Describe(TEXT("IsChildOf"), [this]()
    {
        It(TEXT("正确判断子类"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local GameModeClass = UE.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')\
            return GameModeClass:IsChildOf(UE.AGameModeBase)";
            UnLua::RunChunk(L, Chunk);
            TEST_TRUE(!!lua_toboolean(L, -1));
        });
    });

    Describe(TEXT("GetDefaultObject"), [this]()
    {
        It(TEXT("正确获取类默认对象"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const UObject* Expected = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Core/Blueprints/BP_Game.BP_Game_C"))->GetDefaultObject();
            const char* Chunk = "\
            local GameModeClass = UE.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')\
            return GameModeClass:GetDefaultObject()";
            UnLua::RunChunk(L, Chunk);
            const UObject* Actual = static_cast<UObject*>(UnLua::GetPointer(L, -1));
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
