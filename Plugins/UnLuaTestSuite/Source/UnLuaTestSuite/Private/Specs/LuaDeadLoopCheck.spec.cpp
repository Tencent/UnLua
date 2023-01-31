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

#include "UnLuaTestHelpers.h"
#include "UnLuaModule.h"
#include "UnLuaSettings.h"
#include "UnLuaTemplate.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FLuaDeadLoopCheckSpec, "UnLua.Settings", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

    virtual bool SuppressLogWarnings() override
    {
        return true;
    }

END_DEFINE_SPEC(FLuaDeadLoopCheckSpec)

void FLuaDeadLoopCheckSpec::Define()
{
    Describe(TEXT("DeadLoopCheck"), [this]()
    {
        AfterEach(EAsyncExecution::TaskGraphMainThread, [this]()
        {
            auto& Settings = *GetMutableDefault<UUnLuaSettings>();
            Settings.DeadLoopCheck = 0;
        });

        It(TEXT("设置防止无限循环的超时时间"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            auto& Settings = *GetMutableDefault<UUnLuaSettings>();
            Settings.DeadLoopCheck = 1;

            UnLua::Startup();

            const auto Env = IUnLuaModule::Get().GetEnv();
            AddExpectedError(TEXT("timeout"), EAutomationExpectedErrorFlags::Contains);
            const auto Chunk = R"(
                local count = 0
                while true do
                    count = count + 1
                end
            )";
            Env->DoString(Chunk);

            UnLua::Shutdown();
        });
    });
}

#endif
