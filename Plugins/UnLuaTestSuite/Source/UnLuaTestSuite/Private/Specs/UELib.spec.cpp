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

#include "UnLuaBase.h"
#include "UnLuaTemplate.h"
#include "UnLuaTestHelpers.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS


BEGIN_DEFINE_SPEC(FUELibSpec, "UnLua.API.UE", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
END_DEFINE_SPEC(FUELibSpec)

void FUELibSpec::Define()
{
    Describe(TEXT("全局UE命名空间"), [this]()
    {
        It(TEXT("访问原生类"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::FLuaEnv Env;
            const auto Result = Env.DoString("return UE.FVector");
            TEST_TRUE(Result);
        });
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
