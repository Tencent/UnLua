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

#include "DelegateRegistry.h"
#include "LuaDelegateHandler.h"
#include "lauxlib.h"
#include "UEObjectReferencer.h"
#include "LuaEnv.h"


UnLua::FDelegateRegistry::FDelegateRegistry(lua_State* GL)
    : GL(GL)
{
}

#pragma region FScriptDelgate

void UnLua::FDelegateRegistry::Register(void* Delegate, FProperty* Property)
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

void UnLua::FDelegateRegistry::Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* Lifecycle)
{
    check(lua_type(L, Index) == LUA_TFUNCTION);
    auto& Env = FLuaEnv::FindEnvChecked(GL);
    lua_pushvalue(L, Index);
    const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX); // TODO: release
    LuaFunctions.Add(lua_topointer(L, Index), Ref);
    const auto Handler = ULuaDelegateHandler::CreateFrom(&Env, Ref, Lifecycle);
    GObjectReferencer.AddObjectRef(Handler);
    auto Info = Delegates.FindChecked(Delegate);
    Handler->BindTo(Info.DelegateProperty, Delegate);
    Info.Handlers.Add(Ref, Handler);
}

void UnLua::FDelegateRegistry::Unbind(FScriptDelegate* Delegate)
{
    FDelegateInfo Info = Delegates.FindAndRemoveChecked(Delegate);
    for (auto Pair : Info.Handlers)
    {
        lua_rawgeti(GL, LUA_REGISTRYINDEX, Pair.Key);
        const auto Ref = LuaFunctions.FindAndRemoveChecked(lua_topointer(GL, -1));
        luaL_unref(GL, LUA_REGISTRYINDEX, Ref);
    }
    Delegate->Unbind();
    delete Info.Desc; // TODO: optimize this and release handler
}

void UnLua::FDelegateRegistry::Execute(const ULuaDelegateHandler* Handler, void* Params)
{
    if (!Handler->IsAlive())
        return;

    const auto SignatureDesc = GetSignatureDesc(Handler->Delegate);
    check(SignatureDesc);

    const auto L = Handler->Env->GetMainState();
    SignatureDesc->CallLua(L, Handler->LuaRef, Params, Handler->Lifecycle.Get());
}

int32 UnLua::FDelegateRegistry::Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex)
{
    const auto SignatureDesc = GetSignatureDesc(Delegate);
    check(SignatureDesc);

    const auto Ret = SignatureDesc->ExecuteDelegate(L, NumParams, FirstParamIndex, Delegate);
    return Ret;
}

#pragma endregion

#pragma region FMulticastScriptDelegate

void UnLua::FDelegateRegistry::Add(lua_State* L, int32 Index, void* Delegate, UObject* Lifecycle)
{
    check(lua_type(L, Index) == LUA_TFUNCTION);
    auto& Env = FLuaEnv::FindEnvChecked(GL);
    lua_pushvalue(L, Index);
    const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX); // TODO: release
    LuaFunctions.Add(lua_topointer(L, Index), Ref);
    const auto Handler = ULuaDelegateHandler::CreateFrom(&Env, Ref, Lifecycle);
    Handler->AddTo(Delegates.FindChecked(Delegate).MulticastProperty, Delegate);
    GObjectReferencer.AddObjectRef(Handler);
    Delegates.FindChecked(Delegate).Handlers.Add(Ref, Handler);
}

void UnLua::FDelegateRegistry::Remove(lua_State* L, void* Delegate, int Index)
{
    check(lua_type(L, Index) == LUA_TFUNCTION);
    const auto Ref = LuaFunctions.FindAndRemoveChecked(lua_topointer(L, Index));
    luaL_unref(L, LUA_REGISTRYINDEX, Ref);
    auto Info = Delegates.FindChecked(Delegate);
    const auto Handler = Info.Handlers.FindAndRemoveChecked(Ref);
    GObjectReferencer.RemoveObjectRef(Handler.Get());
    Handler->RemoveFrom(Info.MulticastProperty, Delegate);
}

void UnLua::FDelegateRegistry::Broadcast(lua_State* L, void* Delegate, int32 NumParams, int32 FirstParamIndex)
{
    const auto SignatureDesc = GetSignatureDesc(Delegate);
    check(SignatureDesc);

    const auto Property = Delegates.Find(Delegate)->MulticastProperty;
    const auto ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, Delegate);
    SignatureDesc->BroadcastMulticastDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);
}

void UnLua::FDelegateRegistry::Clear(lua_State* L, void* Delegate)
{
    const auto Info = Delegates.Find(Delegate);
    if (!Info)
        return;

    for (const auto Pair : Info->Handlers)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, Pair.Key);
        Pair.Value->RemoveFrom(Info->MulticastProperty, Delegate);
    }

    delete Info->Desc; // TODO: optimize this and release handler
}

#pragma endregion

FFunctionDesc* UnLua::FDelegateRegistry::GetSignatureDesc(const void* Delegate)
{
    const auto Info = Delegates.Find(Delegate);
    if (!Info)
        return nullptr;
    if (!Info->Desc)
        Info->Desc = new FFunctionDesc(Info->SignatureFunction, nullptr);
    return Info->Desc;
}
