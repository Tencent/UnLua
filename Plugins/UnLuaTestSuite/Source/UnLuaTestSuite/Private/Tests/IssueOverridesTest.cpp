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


#include "IssueOverridesTest.h"
#include "TestCommands.h"
#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_TESTSUITE(FIssueOverridesTest, TEXT("UnLua.Regression.IssueOverrides 覆写引起的问题"))

    bool FIssueOverridesTest::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/IssueOverrides/IssueOverrides.IssueOverrides");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(0.5));
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Counter");
            const auto Result = (int)lua_tointeger(L, -1);
            TestEqual(TEXT("Counter"), Result, 4);

            const auto Obj = NewObject<UIssueOverridesObject>();
            Obj->AddToRoot();

            UnLua::PushUObject(L, Obj);
            lua_setglobal(L, "G_IssueObject");
            
            UnLua::RunChunk(L, "return G_IssueObject:CollectInfo()");
            const auto Result1 = (int32)lua_tointeger(L, -1);
            TestEqual(TEXT("Result1"), Result1, 2);
            
            UnLua::RunChunk(L, "return G_IssueObject:GetConfig()");
            const auto Result2 = (int32)lua_tointeger(L, -1);
            TestEqual(TEXT("Result2"), Result2, 1);

            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssueOverridesTest)

#endif
