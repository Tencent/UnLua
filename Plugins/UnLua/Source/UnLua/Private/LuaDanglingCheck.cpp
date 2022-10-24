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
    FDanglingCheck::FGuard::FGuard(FDanglingCheck* Owner)
        : Owner(Owner)
    {
        Owner->GuardCount++;
    }

    FDanglingCheck::FGuard::~FGuard()
    {
        Owner->GuardCount--;

        if (Owner->Captured.Num() > 0)
        {
            const auto L = Owner->Env->GetMainState();
            lua_getfield(L, LUA_REGISTRYINDEX, "StructMap");
            for (const auto& Pair : Owner->Captured)
            {
                lua_pushlightuserdata(L, Pair.Key);
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

                lua_pushlightuserdata(L, Pair.Key);
                lua_pushnil(L);
                lua_rawset(L, -3);
            }
            lua_pop(L, 1);
            if (Owner->GuardCount == 0)
                Owner->Captured.Empty();
        }

        if (Owner->CapturedContainers.Num() > 0)
        {
            const auto L = Owner->Env->GetMainState();
            lua_getfield(L, LUA_REGISTRYINDEX, "ScriptContainerMap");
            for (const auto& Pair : Owner->CapturedContainers)
            {
                lua_pushlightuserdata(L, Pair.Key);
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

                lua_pushlightuserdata(L, Pair.Key);
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
        return MakeUnique<FGuard>(this);
    }

    void FDanglingCheck::Capture(lua_State* L, void* Value)
    {
        if (!GuardCount)
            return;

        luaL_traceback(L, L, nullptr, 0);
        const auto Stack = UTF8_TO_TCHAR(lua_tostring(L, -1));
        lua_pop(L, 1);
        Captured.Add(Value, Stack);
    }

    void FDanglingCheck::CaptureContainer(lua_State* L, void* Value)
    {
        if (!GuardCount)
            return;

        luaL_traceback(L, L, nullptr, 0);
        const auto Stack = UTF8_TO_TCHAR(lua_tostring(L, -1));
        lua_pop(L, 1);
        CapturedContainers.Add(Value, Stack);
    }
}
