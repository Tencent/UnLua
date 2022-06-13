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

BEGIN_TESTSUITE(FIssue455Test, TEXT("UnLua.Regression.Issue455 CacheScriptContainer导致的崩溃"))

    bool FIssue455Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue455/Issue455");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Result");
            const auto Result = lua_tostring(L, -1);
            const auto Result1 = strcmp(Result, "1A") == 0;
            const auto Result2 = strcmp(Result, "2A") == 0;
            TestTrue(TEXT("Result"), Result1 || Result2);
            return true;
            }, 2.0f));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue455Test)

#endif
