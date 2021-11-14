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

#include "UnLuaTestCommon.h"
#include "Tests/AutomationCommon.h"
#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

bool FUnLuaTestBase::SetUp()
{
    UnLua::Startup();
    L = UnLua::GetState();

#if WITH_EDITOR
    const auto& MapName = GetMapName();
    if (!MapName.IsEmpty())
    {
        AutomationOpenMap(MapName);
        ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand);
        return true;
    }
#endif

    return true;
}

bool FUnLuaTestBase::Update()
{
    FAITestBase::Update();
    return true;
}

void FUnLuaTestBase::TearDown()
{
    FAITestBase::TearDown();

    if (InstantTest())
    {
        if (L)
        {
            UnLua::Shutdown();
            L = nullptr;
        }
    }
    else
    {
#if WITH_EDITOR
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
#endif

        AddLatent([&]()
        {
            if (L)
            {
                UnLua::Shutdown();
                L = nullptr;
            }
        });
    }
}

void FUnLuaTestBase::AddLatent(TFunction<void()>&& Func, float Delay) const
{
    ADD_LATENT_AUTOMATION_COMMAND(FUnLuaTestDelayedCallbackLatentCommand(MoveTemp(Func), Delay));
}
