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

#include "TestCommands.h"
#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_TESTSUITE(FIssue430Test, TEXT("UnLua.Regression.Issue430 动画通知调用组件Lua覆写函数崩溃"))

    bool FIssue430Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue430/Issue430");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();

            lua_getglobal(L, "Result1");
            const auto Result1 = (int32)lua_tointeger(L, -1);
            lua_getglobal(L, "Result2");
            const auto Result2 = (int32)lua_tointeger(L, -1);
            lua_getglobal(L, "Result3");
            const auto Result3 = (int32)lua_tointeger(L, -1);

            TestEqual(TEXT("Result1"), Result1, 2);
            TestEqual(TEXT("Result2"), Result2, 2);
            TestEqual(TEXT("Result3"), Result3, 2);

            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue430Test)

#endif
