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
 * Helper function to get delegate, target UObject and Lua function
 */
template <typename T>
static bool GetMulticastDelegateInfo(lua_State *L, T* &Delegate, UObject* &Object, const void* &CallbackFunction)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    Delegate = (T*)GetCppInstanceFast(L, 1);
    if (!Delegate)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid multicast delegate!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    Object = UnLua::GetUObject(L, 2);
    if (!Object)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    CallbackFunction = lua_topointer(L, 3);
    if (!CallbackFunction)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid function!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("GetMulticastDelegateInfo : %p,%p,%s"), Delegate, Object, *Object->GetName());
#endif
    
    return true;
}

template <typename T>
struct TMulticastDelegateLib
{
    /**
     * Add a callback to the delegate. Parameters must be a UObject and a Lua function
     */
    static int32 Add(lua_State *L)
    {
        T *Delegate = nullptr;
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        bool bSuccess = GetMulticastDelegateInfo(L, Delegate, Object, CallbackFunction);
        if (!bSuccess)
        {
            return 0;
        }

        const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
        Registry->Add(L, 3, Delegate, Object);
        return 0;
    }

    /**
     * Remove a callback from the delegate. Parameters must be a UObject and a Lua function
     */
    static int32 Remove(lua_State *L)
    {
        T *Delegate = nullptr;
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        bool bSuccess = GetMulticastDelegateInfo(L, Delegate, Object, CallbackFunction);
        if (!bSuccess)
        {
            return 0;
        }

        const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
        Registry->Remove(L, Object, (FMulticastScriptDelegate*)Delegate, 3);
        return 0;
    }

    /**
     * Clear all callbacks for the delegate
     */
    static int32 Clear(lua_State *L)
    {
        int32 NumParams = lua_gettop(L);
        if (NumParams != 1)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        T *Delegate = (T*)GetCppInstanceFast(L, 1);
        if (!Delegate)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid multicast delegate!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
        Registry->Clear((FMulticastScriptDelegate*)Delegate);
        return 0;
    }

    /**
     * Call all callbacks bound to the delegate
     */
    static int32 Broadcast(lua_State *L)
    {
        int32 NumParams = lua_gettop(L);
        if (NumParams < 1)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        T *Delegate = (T*)GetCppInstanceFast(L, 1);
        if (!Delegate)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid multicast delegate!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
        Registry->Broadcast(L, Delegate, NumParams - 1, 2);
        return 0;
    }

    static int32 IsBound(lua_State *L)
    {
        int32 NumParams = lua_gettop(L);
        if (NumParams < 1)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        T *Delegate = (T*)GetCppInstanceFast(L, 1);
        if (!Delegate)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid multicast delegate!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        auto Bound = Delegate->IsBound();
        lua_pushboolean(L, Bound);
        return 1;
    }
};

static const luaL_Reg FMulticastScriptDelegateLib[] =
{
    { "Add", TMulticastDelegateLib<FMulticastScriptDelegate>::Add },
    { "Remove", TMulticastDelegateLib<FMulticastScriptDelegate>::Remove },
    { "Clear", TMulticastDelegateLib<FMulticastScriptDelegate>::Clear },
    { "Broadcast", TMulticastDelegateLib<FMulticastScriptDelegate>::Broadcast },
    { "IsBound", TMulticastDelegateLib<FMulticastScriptDelegate>::IsBound },
    { nullptr, nullptr }
};

EXPORT_UNTYPED_CLASS(FMulticastScriptDelegate, false, FMulticastScriptDelegateLib)
IMPLEMENT_EXPORTED_CLASS(FMulticastScriptDelegate)

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
static const luaL_Reg FMulticastSparseDelegateLib[] =
{
    { "Add", TMulticastDelegateLib<FSparseDelegate>::Add },
    { "Remove", TMulticastDelegateLib<FSparseDelegate>::Remove },
    { "Clear", TMulticastDelegateLib<FSparseDelegate>::Clear },
    { "Broadcast", TMulticastDelegateLib<FSparseDelegate>::Broadcast },
    { "IsBound", TMulticastDelegateLib<FSparseDelegate>::IsBound },
    { nullptr, nullptr }
};

EXPORT_UNTYPED_CLASS(FMulticastSparseDelegate, false, FMulticastSparseDelegateLib)
IMPLEMENT_EXPORTED_CLASS(FMulticastSparseDelegate)
#endif
