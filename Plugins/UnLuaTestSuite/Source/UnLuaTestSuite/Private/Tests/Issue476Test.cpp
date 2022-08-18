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

#include "Blueprint/UserWidget.h"
#include "UnLuaTestCommon.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue476 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        const auto Chunk = R"(
            local Outer = NewObject(UE.UUnLuaTestStub)
            local StubClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Regression/Issue476/BP_UnLuaTestStub_Issue476.BP_UnLuaTestStub_Issue476_C")
            G_Stub = NewObject(StubClass, Outer, "Tests.Regression.Issue476.TestStub")
            return G_Stub
        )";
        UnLua::RunChunk(L, Chunk);

        const FWeakObjectPtr Stub = UnLua::GetUObject(L, -1);
        const FWeakObjectPtr Outer = Stub.Get()->GetOuter();
        CollectGarbage(RF_NoFlags, true);

        if (Stub.IsValid())
            UnLuaTestSuite::PrintReferenceChain(Stub.Get());
        if (Outer.IsValid())
            UnLuaTestSuite::PrintReferenceChain(Outer.Get());

        RUNNER_TEST_FALSE(Stub.IsValid());
        RUNNER_TEST_FALSE(Outer.IsValid());
        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue476, TEXT("UnLua.Regression.Issue476 通过NewObject接口添加的UObject引用无法释放"))

#endif
