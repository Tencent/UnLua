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

BEGIN_TESTSUITE(FIssue453Test, TEXT("UnLua.Regression.Issue453 从Lua按引用传递到蓝图的TArray引用变成了空Array"))

    bool FIssue453Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue453/Issue453");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Result1_BP");
            const auto Result1_BP = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result1_BP"), Result1_BP, 3);

            lua_getglobal(L, "Result1_Lua");
            const auto Result1_Lua = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result1_Lua"), Result1_Lua, 3);

            lua_getglobal(L, "Result2_BP");
            const auto Result2_BP = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result2_BP"), Result2_BP, 3);

            lua_getglobal(L, "Result2_Lua");
            const auto Result2_Lua = static_cast<int32>(lua_tonumber(L, -1));
            TestEqual(TEXT("Result2_Lua"), Result2_Lua, 3);
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue453Test)

#endif
