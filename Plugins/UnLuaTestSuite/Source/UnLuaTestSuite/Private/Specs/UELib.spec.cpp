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

BEGIN_DEFINE_SPEC(FUELibSpec, "UnLua.API.UE", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
    lua_State* L;
END_DEFINE_SPEC(FUELibSpec)

void FUELibSpec::Define()
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

    Describe(TEXT("全局UE命名空间"), [this]()
    {
        It(TEXT("访问反射类"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            Env->DoString("return UE.AActor");
            const auto Result = lua_istable(L, -1);
            TEST_TRUE(Result);
        });

        It(TEXT("访问导出类"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            Env->DoString("return UE.FVector");
            const auto Result = lua_istable(L, -1);
            TEST_TRUE(Result);
        });
        
        It(TEXT("访问反射枚举"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const auto Chunk = R"(
                local Enum = UE.EUnLuaTestEnum
                return Enum.None, Enum.Value2
            )";
            Env->DoString(Chunk);
            const auto Result1 = (EUnLuaTestEnum)lua_tointeger(L, -2);
            const auto Result2 = (EUnLuaTestEnum)lua_tointeger(L, -1);
            TEST_EQUAL(Result1, EUnLuaTestEnum::None);
            TEST_EQUAL(Result2, EUnLuaTestEnum::Value2);
        });
    });
}

#endif
