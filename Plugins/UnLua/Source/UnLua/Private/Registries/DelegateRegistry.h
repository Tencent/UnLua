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

#include "lua.h"
#include "LuaDelegateHandler.h"
#include "ReflectionUtils/FunctionDesc.h"

struct FLuaFunction2
{
	FLuaFunction2(TWeakObjectPtr<UObject> InSelfObject, const void* InLuaFunction)
		: SelfObject(InSelfObject)
		, LuaFunction(InLuaFunction)
	{

	}

	TWeakObjectPtr<UObject> SelfObject;

	const void* LuaFunction;

	friend inline bool operator==(const FLuaFunction2& A, const FLuaFunction2& B)
	{
		return A.LuaFunction == B.LuaFunction && A.SelfObject == B.SelfObject;
	}

	friend inline uint32 GetTypeHash(const FLuaFunction2& Key)
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

        void Register(void* Delegate, FProperty* Property, UObject* Owner);

        void Execute(const ULuaDelegateHandler* Handler, void* Params);

        int32 Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex);

        void Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* SelfObject);

        void Unbind(void* Delegate);

        void Add(lua_State* L, int32 Index, void* Delegate, UObject* SelfObject);

        void Remove(lua_State* L, UObject* SelfObject, void* Delegate, int Index);

        void Broadcast(lua_State* L, void* Delegate, int32 NumParams, int32 FirstParamIndex);

        void Clear(void* Delegate);

        void NotifyHandlerBeginDestroy(const ULuaDelegateHandler* Handler);

    private:
        TSharedPtr<FFunctionDesc> GetSignatureDesc(const void* Delegate);

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
            TMap<FLuaFunction2, TWeakObjectPtr<ULuaDelegateHandler>> LuaFunction2Handler;
            bool bIsMulticast;
        };

        TMap<void*, FDelegateInfo> Delegates;
        FLuaEnv* Env;
        FDelegateHandle PostGarbageCollectHandle;
    };
}
