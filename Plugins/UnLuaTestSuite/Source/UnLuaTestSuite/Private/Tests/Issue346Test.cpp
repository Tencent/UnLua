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

#include "UnLuaSettings.h"
#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue346 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        auto& Settings = *GetMutableDefault<UUnLuaSettings>();
        Settings.ModuleLocatorClass = ULuaModuleLocator_ByPackage::StaticClass();

        // 其他测试用例可能产生了一些绑定对象在内存里，在这里肯定是找不到module的
        GetTestRunner().AddExpectedError(TEXT("not found"), EAutomationExpectedErrorFlags::MatchType::Contains, 0);
        GetTestRunner().AddExpectedError(TEXT("Failed to attach"), EAutomationExpectedErrorFlags::MatchType::Contains, 0);
        UE_LOG(LogTemp, Error, TEXT("dummy not found"));
        UE_LOG(LogTemp, Error, TEXT("dummy Failed to attach"));
        
        FUnLuaTestBase::SetUp();
        
        const auto Chunk = R"(
            local FunctionLibrary = UE.UObject.Load("/UnLuaTestSuite/Tests/Regression/Issue346/BFL_Issue346.BFL_Issue346_C")
            print("FunctionLibrary=", FunctionLibrary, type(FunctionLibrary))
            local Result = FunctionLibrary.Test("lua")
            return Result
        )";
        UnLua::RunChunk(L, Chunk);
        const auto Result = lua_tostring(L, -1);
        RUNNER_TEST_EQUAL(Result, "blueprint and lua");
        return true;
    }

    virtual void TearDown() override
    {
        auto& Settings = *GetMutableDefault<UUnLuaSettings>();
        Settings.ModuleLocatorClass = ULuaModuleLocator::StaticClass();

        FUnLuaTestBase::TearDown();
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue346, TEXT("UnLua.Regression.Issue346 BlueprintFunctionLibrary无法静态绑定到lua文件"))

#endif
