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
        PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddLambda([this]
        {
            this->OnPostGarbageCollect();
        });
    }

    FDelegateRegistry::~FDelegateRegistry()
    {
        for (auto& Pair : Delegates)
        {
            for (auto& HandlerPair : Pair.Value.LuaFunction2Handler)
            {
                Env->AutoObjectReference.Remove(HandlerPair.Value.Get());
            }
        }
        Delegates.Empty();
        FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);
    }

    void FDelegateRegistry::OnPostGarbageCollect()
    {
        TArray<TTuple<void*, FDelegateInfo>> InvalidPairs;
        for (auto& Pair : Delegates)
        {
            if (!Pair.Value.Owner.IsValid())
                InvalidPairs.Add(Pair);
        }

        for (int i = 0; i < InvalidPairs.Num(); i++)
        {
            const auto& Pair = InvalidPairs[i];
            if (Pair.Value.bIsMulticast)
                Clear(Pair.Key);
            else
                Unbind(Pair.Key);
            Delegates.Remove(Pair.Key);
        }
    }

    void FDelegateRegistry::NotifyHandlerBeginDestroy(const ULuaDelegateHandler* Handler)
    {
        const auto L = Env->GetMainState();
        luaL_unref(L, LUA_REGISTRYINDEX, Handler->LuaRef);
    }

#pragma region FScriptDelgate

    void FDelegateRegistry::Register(void* Delegate, FProperty* Property, UObject* Owner)
    {
        check(!Owner || Owner->IsValidLowLevelFast());
        const auto Info = Delegates.Find(Delegate);
        if (Info)
        {
            check(Info->Property == Property);
            Info->Owner = Owner;
        }
        else
        {
            FDelegateInfo NewInfo;
            NewInfo.Property = Property;
            NewInfo.Desc = nullptr;
            if (const auto MulticastProperty = CastField<FMulticastDelegateProperty>(Property))
            {
                NewInfo.SignatureFunction = MulticastProperty->SignatureFunction;
                NewInfo.bIsMulticast = true;
            }
            else if (const auto DelegateProperty = CastField<FDelegateProperty>(Property))
            {
                NewInfo.SignatureFunction = DelegateProperty->SignatureFunction;
                NewInfo.bIsMulticast = false;
            }
            else
            {
                check(false);
            }
            NewInfo.Owner = Owner;
            Delegates.Add(Delegate, NewInfo);
        }
    }

    void FDelegateRegistry::Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* SelfObject)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        const auto LuaFunction = lua_topointer(L, Index);
        auto& Info = Delegates.FindChecked(Delegate);
        if (!Info.Owner.IsValid())
            Info.Owner = SelfObject;

        const auto LuaFunction2 = FLuaFunction2(SelfObject, LuaFunction);
        const auto Exists = Info.LuaFunction2Handler.Find(LuaFunction2);
        if (Exists && Exists->IsValid())
            return;

        lua_pushvalue(L, Index);
        const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX);
        const auto Handler = ULuaDelegateHandler::CreateFrom(Env, Ref, Info.Owner.Get(), SelfObject);
        Handler->BindTo(Delegate);
        Env->AutoObjectReference.Add(Handler);
        Info.LuaFunction2Handler.Add(LuaFunction2, Handler);
    }

    void FDelegateRegistry::Unbind(void* Delegate)
    {
        const auto Info = Delegates.Find(Delegate);
        if (!Info)
            return;

        for (const auto Pair : Info->LuaFunction2Handler)
        {
            const auto Handler = Pair.Value;
            if (!Handler.IsValid())
                continue;
            Env->AutoObjectReference.Remove(Handler.Get());
            if (Info->Owner.IsValid())
                ((FScriptDelegate*)Delegate)->Unbind();
        }
        Info->LuaFunction2Handler.Empty();
    }

    void FDelegateRegistry::Execute(const ULuaDelegateHandler* Handler, void* Params)
    {
        const auto SignatureDesc = GetSignatureDesc(Handler->Delegate);
        if (!SignatureDesc)
            return;

        if (Handler->SelfObject.IsStale())
            return;

        const auto L = Env->GetMainState();
        SignatureDesc->CallLua(L, Handler->LuaRef, Params, Handler->SelfObject.Get());
    }

    int32 FDelegateRegistry::Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex)
    {
        const auto SignatureDesc = GetSignatureDesc(Delegate);
        if (!SignatureDesc)
            return 0;

        const auto Ret = SignatureDesc->ExecuteDelegate(L, NumParams, FirstParamIndex, Delegate);
        return Ret;
    }

#pragma endregion

#pragma region FMulticastScriptDelegate

    void FDelegateRegistry::Add(lua_State* L, int32 Index, void* Delegate, UObject* SelfObject)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        const auto LuaFunction = lua_topointer(L, Index);
        auto& Info = Delegates.FindChecked(Delegate);
        if (!Info.Owner.IsValid())
            Info.Owner = SelfObject;

        const auto LuaFunction2 = FLuaFunction2(SelfObject, LuaFunction);
        const auto Exists = Info.LuaFunction2Handler.Find(LuaFunction2);
        if (Exists && Exists->IsValid())
            return;

        lua_pushvalue(L, Index);
        const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX);
        const auto Handler = ULuaDelegateHandler::CreateFrom(Env, Ref, Info.Owner.Get(), SelfObject);
        Env->AutoObjectReference.Add(Handler);
        Handler->AddTo(Info.MulticastProperty, Delegate);
        Info.LuaFunction2Handler.Add(LuaFunction2, Handler);
    }

    void FDelegateRegistry::Remove(lua_State* L, UObject* SelfObject, void* Delegate, int Index)
    {
        check(lua_type(L, Index) == LUA_TFUNCTION);
        const auto LuaFunction = lua_topointer(L, Index);
        auto& Info = Delegates.FindChecked(Delegate);

        const auto LuaFunction2 = FLuaFunction2(SelfObject, LuaFunction);
        TWeakObjectPtr<ULuaDelegateHandler> Handler;
        if (!Info.LuaFunction2Handler.RemoveAndCopyValue(LuaFunction2, Handler))
            return;
        if (!Handler.IsValid())
            return;
        Handler->RemoveFrom(Info.MulticastProperty, Delegate);
        Env->AutoObjectReference.Remove(Handler.Get());
    }

    void FDelegateRegistry::Broadcast(lua_State* L, void* Delegate, int32 NumParams, int32 FirstParamIndex)
    {
        const auto SignatureDesc = GetSignatureDesc(Delegate);
        check(SignatureDesc);

        const auto Info = Delegates.Find(Delegate);
        const auto Property = Info->MulticastProperty;
        const auto ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, Delegate);
        SignatureDesc->BroadcastMulticastDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);
    }

    void FDelegateRegistry::Clear(void* Delegate)
    {
        const auto Info = Delegates.Find(Delegate);
        if (!Info)
            return;

        for (const auto Pair : Info->LuaFunction2Handler)
        {
            const auto Handler = Pair.Value;
            if (!Handler.IsValid())
                continue;
            if (Info->Owner.IsValid())
                Handler->RemoveFrom(Info->MulticastProperty, Delegate);
            Env->AutoObjectReference.Remove(Handler.Get());
        }
        Info->LuaFunction2Handler.Empty();
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
