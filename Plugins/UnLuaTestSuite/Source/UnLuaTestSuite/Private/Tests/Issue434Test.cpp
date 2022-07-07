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

BEGIN_TESTSUITE(FIssue434Test, TEXT("UnLua.Regression.Issue434 MulticastDelegate创建Handler时和self绑定"))

    bool FIssue434Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue434/Issue434");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Result1");
            const auto Result1 = (int32)lua_tointeger(L, -1);
            lua_getglobal(L, "Result2");
            const auto Result2 = (int32)lua_tointeger(L, -1);
            lua_getglobal(L, "Result3");
            const auto Result3 = (int32)lua_tointeger(L, -1);

            TestTrue("Result1", Result1 > 0);
            TestTrue("Result Equal", Result1 == Result2 && Result2 == Result3);
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue434Test)

#endif
