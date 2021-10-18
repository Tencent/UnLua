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
#include "GameFramework/GameModeBase.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibObjectTest, "UnLua.LuaLib_Object", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)


bool FLibObjectTest::RunTest(const FString& Parameters)
{
    const FString ProjectName = FApp::GetProjectName();
    if (ProjectName != "TPSProject")
        return true;

    TestTrue(TEXT("UnLua Startup must succeed"), UnLua::Startup());
    lua_State* L = UnLua::GetState();
    TestNotNull(TEXT("Lua state must created"), L);

    {
        const UClass* Expected = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Core/Blueprints/BP_Game.BP_Game_C"));
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, "return UE4.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')"));
        const UClass* Actual = static_cast<UClass*>(UnLua::GetPointer(L, -1));
        TestEqual(TEXT("Loaded class must correct"), Actual->GetFullName(), Expected->GetFullName());
    }

    {
        const char* Chunk = "\
        local GameModeClass = UE4.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')\
        return UE4.UClass.IsChildOf(GameModeClass, UE4.AGameModeBase)";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        TestTrue(TEXT("Return value must be true"), !!lua_toboolean(L, -1));
    }

    {
        const UObject* Expected = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Core/Blueprints/BP_Game.BP_Game_C"))->GetDefaultObject();
        const char* Chunk = "\
        local GameModeClass = UE4.UClass.Load('/Game/Core/Blueprints/BP_Game.BP_Game_C')\
        return UE4.UClass.GetDefaultObject(GameModeClass)";
        TestTrue(TEXT("Running lua chunk must succeed"), UnLua::RunChunk(L, Chunk));
        const UObject* Actual = static_cast<UObject*>(UnLua::GetPointer(L, -1));
        TestEqual(TEXT("Loaded CDO must correct"), Actual, Expected);
    }

    UnLua::Shutdown();

    TestNull(TEXT("Lua state must closed"), UnLua::GetState());

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
