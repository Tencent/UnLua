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
#include "UnLuaBase.h"
#include "LuaEnv.h"


UnLua::FDelegateRegistry::FDelegateRegistry(lua_State* GL)
    : GL(GL)
{
}

void UnLua::FDelegateRegistry::Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* Lifecycle)
{
    const auto Type = lua_type(L, Index);
    if (Type == LUA_TFUNCTION)
    {
        auto& Env = FLuaEnv::FindEnvChecked(GL);
        lua_pushvalue(L, Index);
        const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX);
        const auto Handler = NewObject<ULuaDelegateHandler>();
        GObjectReferencer.AddObjectRef(Handler);
        Handler->BindTo(Delegate, &Env, Ref, Lifecycle);
        return;
    }

    if (Type == LUA_TUSERDATA)
    {
        const auto Func = (UFunction*)UnLua::GetUObject(L, -1);
        Delegate->BindUFunction(Func->GetOuter(), Func->GetFName());
        return;
    }
}

void UnLua::FDelegateRegistry::Register(FScriptDelegate* Delegate, FDelegateProperty* Property)
{
    FDelegateInfo Info;
    Info.Property = Property;
    Info.Desc = nullptr;
    Delegates.Add(Delegate, Info);
}

void UnLua::FDelegateRegistry::Execute(const ULuaDelegateHandler* Handler, void* Params, int32 LuaRef)
{
    if (!Handler->IsAlive())
        return;

    const auto SignatureDesc = GetSignatureDesc(Handler->Delegate);
    if (!SignatureDesc)
    {
        // TODO: should not be here
        check(false);
        return;
    }

    const auto L = Handler->Env->GetMainState();
    SignatureDesc->CallLua(L, LuaRef, Params, Handler->Lifecycle.Get());
}

int32 UnLua::FDelegateRegistry::Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex)
{
    auto SignatureDesc = GetSignatureDesc(Delegate);
    if (!SignatureDesc)
    {
        // TODO: should not be here
        check(false);
        return 0;
    }

    const auto Ret = SignatureDesc->ExecuteDelegate(L, NumParams, FirstParamIndex, Delegate);
    return Ret;
}

FFunctionDesc* UnLua::FDelegateRegistry::GetSignatureDesc(const FScriptDelegate* Delegate)
{
    const auto Info = Delegates.Find(Delegate);
    if (!Info)
        return nullptr;
    if (!Info->Desc)
        Info->Desc = new FFunctionDesc(Info->Property->SignatureFunction, nullptr);
    return Info->Desc;
}

void UnLua::FDelegateRegistry::Unbind(FScriptDelegate* Delegate)
{
    FDelegateInfo Info = Delegates.FindAndRemoveChecked(Delegate);
    Delegate->Unbind();
    delete Info.Desc; // TODO: optimize this
}
