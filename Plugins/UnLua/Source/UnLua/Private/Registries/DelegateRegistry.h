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

#include "lua.hpp"
#include "LuaDelegateHandler.h"
#include "ReflectionUtils/FunctionDesc.h"

struct FLuaDelegatePair
{
    FLuaDelegatePair(TWeakObjectPtr<UObject> InSelfObject, const void* InLuaFunction)
        : SelfObject(InSelfObject)
          , LuaFunction(InLuaFunction)
    {
    }

    TWeakObjectPtr<UObject> SelfObject;

    const void* LuaFunction;

    friend inline bool operator==(const FLuaDelegatePair& A, const FLuaDelegatePair& B)
    {
        return A.LuaFunction == B.LuaFunction && A.SelfObject == B.SelfObject;
    }

    friend inline uint32 GetTypeHash(const FLuaDelegatePair& Key)
    {
        uint32 Hash = 0;

        Hash = HashCombine(Hash, GetTypeHash(Key.SelfObject));
        Hash = HashCombine(Hash, GetTypeHash(Key.LuaFunction));

        return Hash;
    }
};

namespace UnLua
{
    class FLuaEnv;

    class FDelegateRegistry
    {
    public:
        explicit FDelegateRegistry(FLuaEnv* Env);

        ~FDelegateRegistry();

        void OnPostGarbageCollect();

        FScriptDelegate* Register(FScriptDelegate* Delegate, FDelegateProperty* Property);

        void Register(void* Delegate, FProperty* Property, UObject* Owner);

        void Execute(const ULuaDelegateHandler* Handler, void* Params);

        int32 Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex);

        void Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* SelfObject);

        void Unbind(void* Delegate);

        void Add(lua_State* L, int32 Index, void* Delegate, UObject* SelfObject);

        void Remove(lua_State* L, UObject* SelfObject, void* Delegate, int Index);

        void Broadcast(lua_State* L, void* Delegate, int32 NumParams, int32 FirstParamIndex);

        void Clear(void* Delegate);

        void NotifyHandlerBeginDestroy(ULuaDelegateHandler* Handler);

    private:
        void CheckSignatureCompatible(lua_State* L, ULuaDelegateHandler* Handler, void* OtherDelegate);

        bool CheckSignatureCompatible(void* ADelegate, void* BDelegate);

        TSharedPtr<FFunctionDesc> GetSignatureDesc(const void* Delegate);

        ULuaDelegateHandler* CreateHandler(int LuaRef, UObject* Owner, UObject* SelfObject);

        struct FDelegateInfo
        {
            union
            {
                FProperty* Property;
                FDelegateProperty* DelegateProperty;
                FMulticastDelegateProperty* MulticastProperty;
            };

            UFunction* SignatureFunction;
            TSharedPtr<FFunctionDesc> Desc;
            TWeakObjectPtr<UObject> Owner;
            TSet<TWeakObjectPtr<ULuaDelegateHandler>> Handlers;
            bool bIsMulticast;
            bool bDeleteOnRemove;
        };

        TMap<void*, FDelegateInfo> Delegates;
        TMap<FLuaDelegatePair, TWeakObjectPtr<ULuaDelegateHandler>> CachedHandlers;
        FLuaEnv* Env;
        FDelegateHandle PostGarbageCollectHandle;
    };
}
