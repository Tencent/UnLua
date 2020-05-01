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

#include "CoreUObject.h"
#include "UnLuaCompatibility.h"

struct FCallbackDesc
{
    FCallbackDesc()
        : Class(nullptr), CallbackFunction(nullptr), Hash(0)
    {}

    FCallbackDesc(UClass *InClass, const void *InCallbackFunction)
        : Class(InClass), CallbackFunction(InCallbackFunction)
    {
        uint32 A = PointerHash(Class);
        uint32 B = PointerHash(InCallbackFunction);
        Hash = HashCombine(A, B);
    }

    FORCEINLINE bool operator==(const FCallbackDesc &Callback) const
    {
        return Class == Callback.Class && CallbackFunction == Callback.CallbackFunction;
    }

    UClass *Class;
    void const *CallbackFunction;
    uint32 Hash;
};

FORCEINLINE uint32 GetTypeHash(const FCallbackDesc &Callback)
{
    return Callback.Hash;
}

struct FSignatureDesc
{
    FSignatureDesc()
        : SignatureFunctionDesc(nullptr), CallbackRef(INDEX_NONE), NumCalls(0), NumBindings(1), bPendingKill(false)
    {}

    void MarkForDelete(bool bIgnoreBindings = false);

    void Execute(UObject *Context, FFrame &Stack, void *RetValueAddress);

    class FFunctionDesc *SignatureFunctionDesc;
    int32 CallbackRef;
    int16 NumCalls;
    uint16 NumBindings : 15;
    uint16 bPendingKill : 1;
};

struct lua_State;

#if ENGINE_MINOR_VERSION < 23
typedef FMulticastScriptDelegate FMulticastDelegateType;
#else
typedef void FMulticastDelegateType;
#endif

class FDelegateHelper
{
public:
    DECLARE_FUNCTION(ProcessDelegate);

    static FName GetBindedFunctionName(const FCallbackDesc &Callback);

    static void PreBind(FScriptDelegate *ScriptDelegate, FDelegateProperty *Property);
    static bool Bind(FScriptDelegate *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef);
    static bool Bind(FScriptDelegate *ScriptDelegate, FDelegateProperty *Property, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef);
    static void Unbind(const FCallbackDesc &Callback);
    static void Unbind(FScriptDelegate *ScriptDelegate);
    static int32 Execute(lua_State *L, FScriptDelegate *ScriptDelegate, int32 NumParams, int32 FirstParamIndex);

    static void PreAdd(FMulticastDelegateType *ScriptDelegate, FMulticastDelegateProperty *Property);
    static bool Add(FMulticastDelegateType *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef);
    static bool Add(FMulticastDelegateType *ScriptDelegate, FMulticastDelegateProperty *Property, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef);
    static void Remove(FMulticastDelegateType *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback);
    static void Clear(FMulticastDelegateType *InScriptDelegate);
    static void Broadcast(lua_State *L, FMulticastDelegateType *InScriptDelegate, int32 NumParams, int32 FirstParamIndex);

    static void AddDelegate(FMulticastDelegateType *ScriptDelegate, FScriptDelegate DynamicDelegate);

    static void CleanUpByFunction(UFunction *Function);
    static void CleanUpByClass(UClass *Class);
    static void Cleanup(bool bFullCleanup);

private:
    static void CreateSignature(UFunction *TemplateFunction, FName FuncName, const FCallbackDesc &Callback, int32 CallbackRef);

    static TMap<FScriptDelegate*, FDelegateProperty*> Delegate2Property;
    static TMap<FMulticastDelegateType*, FMulticastDelegateProperty*> MulticastDelegate2Property;
    static TMap<FScriptDelegate*, FFunctionDesc*> Delegate2Signatures;
    static TMap<FMulticastDelegateType*, FFunctionDesc*> MulticastDelegate2Signatures;
    static TMap<UFunction*, FSignatureDesc*> Signatures;
    static TMap<FCallbackDesc, UFunction*> Callbacks;
    static TMap<UFunction*, FCallbackDesc> Function2Callback;
    static TMap<UClass*, TArray<UFunction*>> Class2Functions;
};
