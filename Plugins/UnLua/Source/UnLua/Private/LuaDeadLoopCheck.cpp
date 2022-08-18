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

#include "LuaDeadLoopCheck.h"
#include "HAL/RunnableThread.h"
#include "UnLuaModule.h"

namespace UnLua
{
    int32 FDeadLoopCheck::Timeout = 0;

    FDeadLoopCheck::FDeadLoopCheck(FLuaEnv* Env)
        : Env(Env)
    {
        Runner = new FRunner();
    }

    TUniquePtr<FDeadLoopCheck::FGuard> FDeadLoopCheck::MakeGuard()
    {
        if (Timeout <= 0)
            return TUniquePtr<FGuard>();
        return MakeUnique<FGuard>(this);
    }

    FDeadLoopCheck::FRunner::FRunner()
        : bRunning(true),
          GuardCounter(0),
          TimeoutCounter(0),
          TimeoutGuard(nullptr)
    {
        Thread = FRunnableThread::Create(this, TEXT("LuaDeadLoopCheck"), 0, TPri_BelowNormal);
    }

    uint32 FDeadLoopCheck::FRunner::Run()
    {
        while (bRunning)
        {
            FPlatformProcess::Sleep(1.0f);
            if (GuardCounter.GetValue() == 0)
                continue;

            TimeoutCounter.Increment();
            if (TimeoutCounter.GetValue() < Timeout)
                continue;

            const auto Guard = TimeoutGuard.load();
            if (Guard)
            {
                TimeoutGuard.store(nullptr);
                Guard->SetTimeout();
            }
        }
        return 0;
    }

    void FDeadLoopCheck::FRunner::Stop()
    {
        bRunning = false;
        TimeoutGuard.store(nullptr);
    }

    void FDeadLoopCheck::FRunner::Exit()
    {
        delete this;
    }

    void FDeadLoopCheck::FRunner::GuardEnter(FGuard* Guard)
    {
        if (GuardCounter.Increment() > 1)
            return;
        TimeoutCounter.Set(0);
        TimeoutGuard.store(Guard);
    }

    void FDeadLoopCheck::FRunner::GuardLeave()
    {
        GuardCounter.Decrement();
    }

    FDeadLoopCheck::FGuard::FGuard(FDeadLoopCheck* Owner)
        : Owner(Owner)
    {
        Owner->Runner->GuardEnter(this);
    }

    FDeadLoopCheck::FGuard::~FGuard()
    {
        Owner->Runner->GuardLeave();
    }

    void FDeadLoopCheck::FGuard::SetTimeout()
    {
        const auto L = Owner->Env->GetMainState();
        const auto Hook = lua_gethook(L);
        if (Hook == nullptr)
            lua_sethook(L, OnLuaLineEvent, LUA_MASKLINE, 0);
    }

    void FDeadLoopCheck::FGuard::OnLuaLineEvent(lua_State* L, lua_Debug* ar)
    {
        lua_sethook(L, nullptr, 0, 0);
        luaL_error(L, "lua script exec timeout");
    }
}
