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

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "lua.h"
#include <atomic>

namespace UnLua
{
    class FLuaEnv;

    class FDeadLoopCheck
    {
    public:
        static int32 Timeout; // in seconds
        
        class FGuard final
        {
        public:
            explicit FGuard(FDeadLoopCheck* Owner);

            ~FGuard();

            void SetTimeout();

        private:
            static void OnLuaLineEvent(lua_State* L, lua_Debug* ar);

            FDeadLoopCheck* Owner;
        };

        class FRunner final : public FRunnable
        {
        public:
            explicit FRunner();

            virtual uint32 Run() override;

            virtual void Stop() override;

            virtual void Exit() override;

            void GuardEnter(FGuard* Guard);

            void GuardLeave();

        private:
            FThreadSafeBool bRunning;
            FRunnableThread* Thread;
            FThreadSafeCounter GuardCounter;
            FThreadSafeCounter TimeoutCounter;
            std::atomic<FGuard*> TimeoutGuard;
        };
        
        explicit FDeadLoopCheck(FLuaEnv* Env);

        TUniquePtr<FGuard> MakeGuard();

    private:
        FRunner* Runner;
        FLuaEnv* Env;
    };
}
