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


BEGIN_DEFINE_SPEC(FLuaEnvSpec, "UnLua.API.FLuaEnv", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<UnLua::FLuaEnv> Env;
END_DEFINE_SPEC(FLuaEnvSpec)

void FLuaEnvSpec::Define()
{
    BeforeEach([this]
    {
        Env = MakeShared<UnLua::FLuaEnv>();
    });

    Describe(TEXT("创建Lua环境"), [this]()
    {
        It(TEXT("支持多个Lua环境"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::FLuaEnv Env1;
            UnLua::FLuaEnv Env2;

            Env1.DoString("return 1");
            Env2.DoString("return 2");

            const auto A = (int32)lua_tointeger(Env1.GetMainState(), -1);
            const auto B = (int32)lua_tointeger(Env2.GetMainState(), -1);

            TEST_EQUAL(A, 1);
            TEST_EQUAL(B, 2);
        });

        It(TEXT("支持启动脚本和参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::FLuaEnv Env1;
            const auto L = Env1.GetMainState();
            TMap<FString, UObject*> Args;
            Args.Add("GWorld", GWorld);
            Env1.Start(TEXT("Tests.Specs.LuaEnv.支持启动脚本和参数"), Args);
            lua_getglobal(L, "GWorld");
            const auto Actual = (UWorld*)UnLua::GetUObject(L, -1);
            const auto Expected = (UWorld*)GWorld;
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        Env.Reset();
    });
}

#endif
