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

#include "Registries/DelegateRegistry.h"
#include "LuaDelegateHandler.h"
#include "lauxlib.h"
#include "ObjectReferencer.h"
#include "LuaEnv.h"

namespace UnLua
{
    FDelegateRegistry::FDelegateRegistry(FLuaEnv* Env)
        : Env(Env)
    {
    }

#pragma region FScriptDelgate

    void FDelegateRegistry::Register(void* Delegate, FProperty* Property)
    {
        const auto Info = Delegates.Find(Delegate);
        if (Info)
        {
            check(Info->Property == Property);
            Info->Property = Property;
        }
        else
        {
            FDelegateInfo NewInfo;
            NewInfo.Property = Property;
            NewInfo.Desc = nullptr;
            if (const auto MulticastProperty = CastField<FMulticastDelegateProperty>(Property))
                NewInfo.SignatureFunction = MulticastProperty->SignatureFunction;
            else if (const auto DelegateProperty = CastField<FDelegateProperty>(Property))
                NewInfo.SignatureFunction = DelegateProperty->SignatureFunction;
            else
                NewInfo.SignatureFunction = nullptr;
            Delegates.Add(Delegate, NewInfo);
        }
    }

    void FDelegateRegistry::Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* Lifecycle)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        lua_pushvalue(L, Index);
        const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX); // TODO: release
        LuaFunctions.Add(lua_topointer(L, Index), Ref);
        const auto Handler = ULuaDelegateHandler::CreateFrom(Env, Ref, Lifecycle);
        Env->AutoObjectReference.Add(Handler);
        auto Info = Delegates.FindChecked(Delegate);
        Handler->BindTo(Info.DelegateProperty, Delegate);
        Info.Handlers.Add(Ref, Handler);
    }

    void FDelegateRegistry::Unbind(FScriptDelegate* Delegate)
    {
        const auto L = Env->GetMainState();
        FDelegateInfo Info = Delegates.FindAndRemoveChecked(Delegate);
        for (const auto Pair : Info.Handlers)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, Pair.Key);
            const auto Ref = LuaFunctions.FindAndRemoveChecked(lua_topointer(L, -1));
            luaL_unref(L, LUA_REGISTRYINDEX, Ref);
        }
        Delegate->Unbind();
    }

    void FDelegateRegistry::Execute(const ULuaDelegateHandler* Handler, void* Params)
    {
        if (!Handler->IsAlive())
            return;

        const auto SignatureDesc = GetSignatureDesc(Handler->Delegate);
        check(SignatureDesc);

        const auto L = Handler->Env->GetMainState();
        SignatureDesc->CallLua(L, Handler->LuaRef, Params, Handler->Lifecycle.Get());
    }

    int32 FDelegateRegistry::Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex)
    {
        const auto SignatureDesc = GetSignatureDesc(Delegate);
        check(SignatureDesc);

        const auto Ret = SignatureDesc->ExecuteDelegate(L, NumParams, FirstParamIndex, Delegate);
        return Ret;
    }

#pragma endregion

#pragma region FMulticastScriptDelegate

    void FDelegateRegistry::Add(lua_State* L, int32 Index, void* Delegate, UObject* Lifecycle)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        lua_pushvalue(L, Index);
        const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX); // TODO: release
        LuaFunctions.Add(lua_topointer(L, Index), Ref);
        const auto Handler = ULuaDelegateHandler::CreateFrom(Env, Ref, Lifecycle);
        Handler->AddTo(Delegates.FindChecked(Delegate).MulticastProperty, Delegate);
        Env->AutoObjectReference.Add(Handler);
        Delegates.FindChecked(Delegate).Handlers.Add(Ref, Handler);
    }

    void FDelegateRegistry::Remove(lua_State* L, void* Delegate, int Index)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        const auto Ref = LuaFunctions.FindAndRemoveChecked(lua_topointer(L, Index));
        luaL_unref(L, LUA_REGISTRYINDEX, Ref);
        auto Info = Delegates.FindChecked(Delegate);
        const auto Handler = Info.Handlers.FindAndRemoveChecked(Ref);
        Env->AutoObjectReference.Remove(Handler.Get());
        Handler->RemoveFrom(Info.MulticastProperty, Delegate);
    }

    void FDelegateRegistry::Broadcast(lua_State* L, void* Delegate, int32 NumParams, int32 FirstParamIndex)
    {
        const auto SignatureDesc = GetSignatureDesc(Delegate);
        check(SignatureDesc);

        const auto Property = Delegates.Find(Delegate)->MulticastProperty;
        const auto ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, Delegate);
        SignatureDesc->BroadcastMulticastDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);
    }

    void FDelegateRegistry::Clear(lua_State* L, void* Delegate)
    {
        const auto Info = Delegates.Find(Delegate);
        if (!Info)
            return;

        for (const auto Pair : Info->Handlers)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, Pair.Key);
            Pair.Value->RemoveFrom(Info->MulticastProperty, Delegate);
        }

        Delegates.Remove(Delegate);
    }

#pragma endregion

    TSharedPtr<FFunctionDesc> FDelegateRegistry::GetSignatureDesc(const void* Delegate)
    {
        const auto Info = Delegates.Find(Delegate);
        if (!Info)
            return nullptr;
        if (!Info->Desc)
            Info->Desc = MakeShared<FFunctionDesc>(Info->SignatureFunction, nullptr);
        return Info->Desc;
    }
}
