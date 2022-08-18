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

#include "UnLuaEx.h"
#include "LuaCore.h"

/**
 * Bind a callback for the delegate. Parameters must be a UObject and a Lua function
 */
static int32 FScriptDelegate_Bind(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
        return luaL_error(L, "invalid parameters");

    FScriptDelegate* Delegate = (FScriptDelegate*)GetCppInstanceFast(L, 1);
    if (!Delegate)
        return luaL_error(L, "invalid dynamic delegate");

    UObject* Object = UnLua::GetUObject(L, 2);
    if (!Object)
        return luaL_error(L, "invalid object");

    if (lua_type(L, 3) != LUA_TFUNCTION)
        return luaL_error(L, "invalid function");

    const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
    Registry->Bind(L, 3, Delegate, Object);

    return 0;
}

/**
 * Unbind the callback for the delegate
 */
static int32 FScriptDelegate_Unbind(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FScriptDelegate* Delegate = (FScriptDelegate*)GetCppInstanceFast(L, 1);
    if (!Delegate)
        return luaL_error(L, "invalid dynamic delegate");

    const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
    Registry->Unbind(Delegate);

    return 0;
}

/**
 * Call the callback bound to the delegate
 */
static int32 FScriptDelegate_Execute(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
        return luaL_error(L, "invalid parameters");

    FScriptDelegate* Delegate = (FScriptDelegate*)GetCppInstanceFast(L, 1);
    if (!Delegate)
        return luaL_error(L, "invalid dynamic delegate");

    const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
    return Registry->Execute(L, Delegate, NumParams + 2, 2);
}

static const luaL_Reg FScriptDelegateLib[] =
{
    {"Bind", FScriptDelegate_Bind},
    {"Unbind", FScriptDelegate_Unbind},
    {"Execute", FScriptDelegate_Execute},
    {nullptr, nullptr}
};

EXPORT_UNTYPED_CLASS(FScriptDelegate, false, FScriptDelegateLib)

IMPLEMENT_EXPORTED_CLASS(FScriptDelegate)
