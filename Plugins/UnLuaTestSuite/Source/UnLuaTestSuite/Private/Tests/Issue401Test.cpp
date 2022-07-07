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

BEGIN_TESTSUITE(FIssue401Test, TEXT("UnLua.Regression.Issue401 LUA覆写导致数组传参错误"))

    bool FIssue401Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue401/Issue401");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Result1");
            const auto Result1 = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result1"), Result1, 2);

            lua_getglobal(L, "Result2");
            const auto Result2 = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result2"), Result2, 2);
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue401Test)

#endif
