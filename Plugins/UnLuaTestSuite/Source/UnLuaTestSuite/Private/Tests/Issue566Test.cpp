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


#include "Issue566Test.h"
#include "LowLevel.h"
#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

TArray<FIssue566Delegate> UIssue566FunctionLibrary::Delegates;

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue566 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk1 = R"(
            local A = NewObject(UE.UIssue566Object)
            local B = NewObject(UE.UIssue566Object)
            _G.Result = false
            _G.Ref = UnLua.Ref(B)
            _G.Callback = function()
                _G.Result = true
            end
            UE.UIssue566FunctionLibrary.Reset()
            UE.UIssue566FunctionLibrary.AddCallback({ B, _G.Callback })
            UE.UIssue566FunctionLibrary.AddCallback({ A, _G.Callback })
        )";
        UnLua::RunChunk(L, Chunk1);

        CollectGarbage(RF_NoFlags, true);

        const auto Chunk2 = R"(
            UE.UIssue566FunctionLibrary.Invoke()
        )";
        UnLua::RunChunk(L, Chunk2);

        lua_getglobal(L, "Result");
        const auto Result = lua_toboolean(L, -1);
        RUNNER_TEST_TRUE(Result);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue566, TEXT("UnLua.Regression.Issue566 多次传递委托类型的参数到同一函数时，可能因为Owner失效而无法回调"))

#endif
