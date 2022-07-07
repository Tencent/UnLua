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
#include "Registries/FunctionRegistry.h"

struct lua_State;
struct FParameterCollection;
class FPropertyDesc;

/**
 * Function descriptor
 */
class FFunctionDesc
{
public:
    FFunctionDesc(UFunction *InFunction, FParameterCollection *InDefaultParams);
    ~FFunctionDesc();

    /**
     * Check the validity of this function
     *
     * @return - true if the function is valid, false otherwise
     */
    FORCEINLINE bool IsValid() const { return Function.IsValid(); }

    /**
     * Test if this function has return property
     *
     * @return - true if the function has return property, false otherwise
     */
    FORCEINLINE bool HasReturnProperty() const { return ReturnPropertyIndex > INDEX_NONE; }

    /**
     * Test if this function is a latent function
     *
     * @return - true if the function is a latent function, false otherwise
     */
    FORCEINLINE bool IsLatentFunction() const { return LatentPropertyIndex > INDEX_NONE; }

    /**
     * Get the number of out properties
     *
     * @return - the number of out properties. out properties means return property or non-const reference properties
     */
    FORCEINLINE uint8 GetNumOutProperties() const { return ReturnPropertyIndex > INDEX_NONE ? OutPropertyIndices.Num() + 1 : OutPropertyIndices.Num(); }

    /**
     * Get the number of reference properties
     *
     * @return - the number of reference properties.
     */
    FORCEINLINE uint8 GetNumRefProperties() const { return NumRefProperties; }

    /**
     * Get the number of non-const reference properties
     *
     * @return - the number of non-const reference properties.
     */
    FORCEINLINE uint8 GetNumNoConstRefProperties() const { return OutPropertyIndices.Num(); }

    /**
     * Get the 'true' function
     *
     * @return - the UFunction
     */
    FORCEINLINE UFunction* GetFunction() const { return Function.Get(); }

    FORCEINLINE const char* GetLuaFunctionName() const { return LuaFunctionName->Get(); }
 
    void CallLua(lua_State* L, lua_Integer FunctionRef, lua_Integer SelfRef, FFrame& Stack, RESULT_DECL);
 
    bool CallLua(lua_State* L, int32 LuaRef, void* Params, UObject* Self);
 
    /**
     * Call this UFunction
     *
     * @param NumParams - the number of parameters
     * @param Userdata - user data, now it's only used for latent function and it must be a 'int32'
     * @return - the number of return values pushed on the stack
     */
    int32 CallUE(lua_State *L, int32 NumParams, void *Userdata = nullptr);

    /**
     * Fire the delegate
     *
     * @param NumParams - the number of parameters
     * @param FirstParamIndex - Lua index of the first parameter
     * @param ScriptDelegate - the delegate
     * @return - the number of return values pushed on the stack
     */
    int32 ExecuteDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FScriptDelegate *ScriptDelegate);

    /**
     * Fire the multicast delegate
     *
     * @param NumParams - the number of parameters
     * @param FirstParamIndex - Lua index of the first parameter
     * @param ScriptDelegate - the multicast delegate
     */
    void BroadcastMulticastDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FMulticastScriptDelegate *ScriptDelegate);

   private:
    void* PreCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, TArray<bool> &CleanupFlags, void *Userdata = nullptr);
    int32 PostCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, void *Params, const TArray<bool> &CleanupFlags);

    bool CallLuaInternal(lua_State *L, void *InParams, FOutParmRec *OutParams, void *RetValueAddress) const;

    TWeakObjectPtr<UFunction> Function;
    FString FuncName;
#if ENABLE_PERSISTENT_PARAM_BUFFER
    void *Buffer;
#endif
#if !SUPPORTS_RPC_CALL
    FOutParmRec *OutParmRec;
#endif
    TArray<TUniquePtr<FPropertyDesc>> Properties;
    TArray<int32> OutPropertyIndices;
    FParameterCollection *DefaultParams;
    int32 ReturnPropertyIndex;
    int32 LatentPropertyIndex;
    uint8 NumRefProperties;
    uint8 NumCalls;                 // RECURSE_LIMIT is 120 or 250 which is less than 256, so use a byte...
    uint8 bStaticFunc : 1;
    uint8 bInterfaceFunc : 1;
    uint8 bHasDelegateParams : 1;
    int32 ParmsSize;
    TUniquePtr<FTCHARToUTF8> LuaFunctionName;
};
