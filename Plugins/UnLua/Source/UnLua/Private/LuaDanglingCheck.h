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
#include "lua.hpp"

namespace UnLua
{
    class FLuaEnv;

    class FDanglingCheck
    {
    public:
        static bool Enabled;

        class FGuard final
        {
        public:
            explicit FGuard(FDanglingCheck* Owner);

            ~FGuard();

        private:
            FDanglingCheck* Owner;
        };

        explicit FDanglingCheck(FLuaEnv* Env);

        TUniquePtr<FGuard> MakeGuard();

        void CaptureStruct(lua_State* L, void* Value);

        void CaptureContainer(lua_State* L, void* Value);

    private:
        FLuaEnv* Env;
        int32 GuardCount;
        TSet<void*> CapturedStructs;
        TSet<void*> CapturedContainers;
    };
}
