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

#include "LuaDanglingCheck.h"
#include "LowLevel.h"
#include "LuaEnv.h"
#include "UnLuaDebugBase.h"

namespace UnLua
{
    bool FDanglingCheck::Enabled;

    FDanglingCheck::FGuard::FGuard(FDanglingCheck* Owner)
        : Owner(Owner)
    {
        Owner->GuardCount++;
    }

    FDanglingCheck::FGuard::~FGuard()
    {
        Owner->GuardCount--;

        if (Owner->CapturedStructs.Num() > 0)
        {
            const auto L = Owner->Env->GetMainState();
            lua_getfield(L, LUA_REGISTRYINDEX, "StructMap");
            for (const auto& StructPtr : Owner->CapturedStructs)
            {
                lua_pushlightuserdata(L, StructPtr);
                lua_rawget(L, -2);
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    continue;
                }

                check(lua_isuserdata(L, -1))
                bool TwoLevelPtr;
                void* Userdata = GetUserdataFast(L, -1, &TwoLevelPtr);
                check(TwoLevelPtr)
                *(void**)Userdata = nullptr;

                lua_pop(L, 1);

                lua_pushlightuserdata(L, StructPtr);
                lua_pushnil(L);
                lua_rawset(L, -3);
            }
            lua_pop(L, 1);
            if (Owner->GuardCount == 0)
                Owner->CapturedStructs.Empty();
        }

        if (Owner->CapturedContainers.Num() > 0)
        {
            const auto L = Owner->Env->GetMainState();
            lua_getfield(L, LUA_REGISTRYINDEX, "ScriptContainerMap");
            for (const auto& ContainerPtr : Owner->CapturedContainers)
            {
                lua_pushlightuserdata(L, ContainerPtr);
                lua_rawget(L, -2);
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    continue;
                }

                bool TwoLevelPtr;
                void* Userdata = GetUserdataFast(L, -1, &TwoLevelPtr);
                SetUserdataFlags(Userdata, 1 << 6); // TODO:BIT_RELEASED_TAG
                check(!TwoLevelPtr)

                lua_pop(L, 1);

                lua_pushlightuserdata(L, ContainerPtr);
                lua_pushnil(L);
                lua_rawset(L, -3);
            }
            lua_pop(L, 1);
            if (Owner->GuardCount == 0)
                Owner->CapturedContainers.Empty();
        }
    }

    FDanglingCheck::FDanglingCheck(FLuaEnv* Env)
        : Env(Env), GuardCount(0)
    {
    }

    TUniquePtr<FDanglingCheck::FGuard> FDanglingCheck::MakeGuard()
    {
        if (!Enabled)
            return TUniquePtr<FGuard>();
        return MakeUnique<FGuard>(this);
    }

    void FDanglingCheck::CaptureStruct(lua_State* L, void* Value)
    {
        if (!GuardCount)
            return;
        CapturedStructs.Add(Value);
    }

    void FDanglingCheck::CaptureContainer(lua_State* L, void* Value)
    {
        if (!GuardCount)
            return;
        CapturedContainers.Add(Value);
    }
}
