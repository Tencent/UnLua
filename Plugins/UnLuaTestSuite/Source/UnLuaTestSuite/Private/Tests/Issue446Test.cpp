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
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_TESTSUITE(FIssue446Test, TEXT("UnLua.Regression.Issue446 实现了FTickableGameObject的对象在Tick里调用自身被Lua覆写的方法会崩溃"))

    bool FIssue446Test::RunTest(const FString& Parameters)
    {
        static UUnLuaTestStubForIssue446* Stub;
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Misc/Empty");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            Stub = NewObject<UUnLuaTestStubForIssue446>();
            Stub->AddToRoot();
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            lua_getglobal(L, "Issue446_Result");
            const auto Result = (bool)lua_toboolean(L, -1);
            TestTrue("Issue446_Result", Result);
            Stub->RemoveFromRoot();
            CollectGarbage(RF_NoFlags, true);
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FIssue446Test)

#endif
