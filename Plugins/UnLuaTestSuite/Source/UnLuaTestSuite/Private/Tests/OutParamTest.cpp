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

#include "OutParamTest.h"

#include "Engine.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_OutParamFull, TEXT("UnLua.API.OutParam.Full 返回全部Out参数"), EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_OutParamFull::RunTest(const FString& Parameters)
{
    UnLua::Startup();

    const auto Stub = NewObject<UOutParamTestStub>();

    FVector Dest = FVector::OneVector;
    UObject* Pkg = GetTransientPackage();
    float Radius = 1.0f;
    bool bStop = false;
    Stub->ReturnFull(Dest, Pkg, Radius, bStop);

    FVector ExpectedVector = FVector::ZeroVector;
    UObject* ExpectedPkg = nullptr;
    float ExpectedRadius = 2.0f;
    bool ExpectedStop = true;

    TEST_EQUAL(Dest, ExpectedVector);
    TEST_EQUAL(Pkg, ExpectedPkg);
    TEST_EQUAL(Radius, ExpectedRadius);
    TEST_EQUAL(bStop, ExpectedStop);

    UnLua::Shutdown();

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnLuaTest_OutParamPartial, TEXT("UnLua.API.OutParam.Partial 返回部分Out参数，剩余部分使用传入参数填充"),
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FUnLuaTest_OutParamPartial::RunTest(const FString& Parameters)
{
    UnLua::Startup();

    const auto Stub = NewObject<UOutParamTestStub>();

    FVector ExpectedVector = FVector::OneVector;
    UObject* ExpectedPkg = GetTransientPackage();
    float ExpectedRadius = 1.0f;
    bool ExpectedStop = true;

    FVector Dest = ExpectedVector;
    UObject* Pkg = ExpectedPkg;
    float Radius = ExpectedRadius;
    bool bStop = ExpectedStop;
    Stub->ReturnPartial(Dest, Pkg, Radius, bStop);

    TEST_EQUAL(Dest, ExpectedVector);
    TEST_EQUAL(Radius, ExpectedRadius);
    TEST_EQUAL(bStop, ExpectedStop);
    TEST_EQUAL(Pkg, ExpectedPkg);

    UnLua::Shutdown();

    return true;
}

#endif
