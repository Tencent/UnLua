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

#include "FunctionDesc.h"
#include "PropertyDesc.h"
#include "ReflectionRegistry.h"
#include "LuaCore.h"
#include "LuaContext.h"
#include "LuaFunctionInjection.h"
#include "DefaultParamCollection.h"

/**
 * Function descriptor constructor
 */
FFunctionDesc::FFunctionDesc(UFunction *InFunction, FParameterCollection *InDefaultParams, int32 InFunctionRef)
    : Function(InFunction), DefaultParams(InDefaultParams), ReturnPropertyIndex(INDEX_NONE), LatentPropertyIndex(INDEX_NONE)
    , FunctionRef(InFunctionRef), NumRefProperties(0), NumCalls(0), bStaticFunc(false), bInterfaceFunc(false)
{
    check(InFunction);

    FuncName = InFunction->GetName();

    bStaticFunc = InFunction->HasAnyFunctionFlags(FUNC_Static);         // a static function?

    UClass *OuterClass = InFunction->GetOuterUClass();
    if (OuterClass->HasAnyClassFlags(CLASS_Interface) && OuterClass != UInterface::StaticClass())
    {
        bInterfaceFunc = true;                                          // a function in interface?
    }

    // create persistent parameter buffer. memory for speed
#if ENABLE_PERSISTENT_PARAM_BUFFER
    Buffer = nullptr;
    if (InFunction->ParmsSize > 0)
    {
        Buffer = FMemory::Malloc(InFunction->ParmsSize, 16);
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        INC_MEMORY_STAT_BY(STAT_UnLua_PersistentParamBuffer_Memory, Size);
#endif
    }
#endif

    // pre-create OutParmRec. memory for speed
#if !SUPPORTS_RPC_CALL
    OutParmRec = nullptr;
    FOutParmRec *CurrentOutParmRec = nullptr;
#endif

    static const FName NAME_LatentInfo = TEXT("LatentInfo");
    Properties.Reserve(InFunction->NumParms);
    for (TFieldIterator<FProperty> It(InFunction); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        FProperty *Property = *It;
        int32 Index = Properties.Add(FPropertyDesc::Create(Property));
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            ReturnPropertyIndex = Index;                                // return property
        }
        else if (LatentPropertyIndex == INDEX_NONE && Property->GetFName() == NAME_LatentInfo)
        {
            LatentPropertyIndex = Index;                                // 'LatentInfo' property for latent function
        }
        else if (Property->HasAnyPropertyFlags(CPF_OutParm))
        {
            ++NumRefProperties;

            // pre-create OutParmRec for 'out' property
#if !SUPPORTS_RPC_CALL
            FOutParmRec *Out = (FOutParmRec*)FMemory::Malloc(sizeof(FOutParmRec), alignof(FOutParmRec));
#if STATS
            const uint32 Size = FMemory::GetAllocSize(Out);
            INC_MEMORY_STAT_BY(STAT_UnLua_OutParmRec_Memory, Size);
#endif
            Out->PropAddr = Property->ContainerPtrToValuePtr<uint8>(Buffer);
            Out->Property = Property;
            if (CurrentOutParmRec)
            {
                CurrentOutParmRec->NextOutParm = Out;
                CurrentOutParmRec = Out;
            }
            else
            {
                OutParmRec = Out;
                CurrentOutParmRec = Out;
            }
#endif

            if (!Property->HasAnyPropertyFlags(CPF_ConstParm))
            {
                OutPropertyIndices.Add(Index);                          // non-const reference property
            }
        }
    }

#if !SUPPORTS_RPC_CALL
    if (CurrentOutParmRec)
    {
        CurrentOutParmRec->NextOutParm = nullptr;
    }
#endif
}

/**
 * Function descriptor destructor
 */
FFunctionDesc::~FFunctionDesc()
{
    // free persistent parameter buffer
#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (Buffer)
    {
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        DEC_MEMORY_STAT_BY(STAT_UnLua_PersistentParamBuffer_Memory, Size);
#endif
        FMemory::Free(Buffer);
    }
#endif

    // free pre-created OutParmRec
#if !SUPPORTS_RPC_CALL
    while (OutParmRec)
    {
        FOutParmRec *NextOut = OutParmRec->NextOutParm;
#if STATS
        const uint32 Size = FMemory::GetAllocSize(OutParmRec);
        DEC_MEMORY_STAT_BY(STAT_UnLua_OutParmRec_Memory, Size);
#endif
        FMemory::Free(OutParmRec);
        OutParmRec = NextOut;
    }
#endif

    // release cached property descriptors
    for (FPropertyDesc *Property : Properties)
    {
        delete Property;
    }

    // remove Lua reference for this function
    if (FunctionRef != INDEX_NONE)
    {
        luaL_unref(*GLuaCxt, LUA_REGISTRYINDEX, FunctionRef);
    }
}

/**
 * Call Lua function that overrides this UFunction
 */
bool FFunctionDesc::CallLua(UObject *Context, FFrame &Stack, void *RetValueAddress, bool bRpcCall, bool bUnpackParams)
{
    // push Lua function to the stack
    bool bSuccess = false;
    lua_State *L = *GLuaCxt;
    if (FunctionRef != INDEX_NONE)
    {
        bSuccess = PushFunction(L, Context, FunctionRef);
    }
    else
    {
        FunctionRef = PushFunction(L, Context, bRpcCall ? TCHAR_TO_ANSI(*FString::Printf(TEXT("%s_RPC"), *FuncName)) : TCHAR_TO_ANSI(*FuncName));
        bSuccess = FunctionRef != INDEX_NONE;
    }

    if (bSuccess)
    {
        if (bUnpackParams)
        {
#if ENABLE_PERSISTENT_PARAM_BUFFER
            void *Params = Buffer;
#else
            void *Params = Function->ParmsSize > 0 ? FMemory::Malloc(Function->ParmsSize, 16) : nullptr;
#endif

            for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm) == CPF_Parm; ++It)
            {
                Stack.Step(Stack.Object, It->ContainerPtrToValuePtr<uint8>(Params));
            }
            check(Stack.PeekCode() == EX_EndFunctionParms);
            Stack.SkipCode(1);          // skip EX_EndFunctionParms

            bSuccess = CallLuaInternal(L, Params, Stack.OutParms, RetValueAddress);             // call Lua function...

#if !ENABLE_PERSISTENT_PARAM_BUFFER
            FMemory::Free(Params);
#endif
        }
        else
        {
            bSuccess = CallLuaInternal(L, Stack.Locals, Stack.OutParms, RetValueAddress);       // call Lua function...
        }
    }

    return bSuccess;
}

/**
 * Call the UFunction
 */
int32 FFunctionDesc::CallUE(lua_State *L, int32 NumParams, void *Userdata)
{
    check(Function);

    int32 FirstParamIndex = 1;
    UObject *Object = nullptr;
    if (bStaticFunc)
    {
        UClass *OuterClass = Function->GetOuterUClass();
        Object = OuterClass->GetDefaultObject();                // get CDO for static function
    }
    else
    {
        check(NumParams > 0);
        Object = UnLua::GetUObject(L, 1);
        ++FirstParamIndex;
        --NumParams;
    }

    if (!Object)
    {
        UE_LOG(LogUnLua, Warning, TEXT("!!! NULL target object for UFunction '%s'! Check the usage of ':' and '.'!"), *FuncName);
        return 0;
    }

#if SUPPORTS_RPC_CALL
    int32 Callspace = Object->GetFunctionCallspace(Function, nullptr);
    bool bRemote = Callspace & FunctionCallspace::Remote;
#else
    bool bRemote = false;
#endif

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Userdata);      // prepare values of properties

    UFunction *FinalFunction = Function;
    if (bInterfaceFunc)
    {
        // get target UFunction if it's a function in Interface
        FName FunctionName = Function->GetFName();
        FinalFunction = Object->GetClass()->FindFunctionByName(FunctionName);
        if (!FinalFunction)
        {
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("ERROR! Can't find UFunction '%s' in target object!"), *FuncName);
            return 0;
        }
#if UE_BUILD_DEBUG
        else if (FinalFunction != Function)
        {
            // todo: 'FinalFunction' must have the same signature with 'Function', check more parameters here
            check(FinalFunction->NumParms == Function->NumParms && FinalFunction->ParmsSize == Function->ParmsSize && FinalFunction->ReturnValueOffset == Function->ReturnValueOffset);
        }
#endif
    }
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    else
    {
        if (IsOverridable(Function) && !bRemote)
        {
            UFunction *OverriddenFunc = GReflectionRegistry.FindOverriddenFunction(Function);
            if (OverriddenFunc)
            {
                FinalFunction = OverriddenFunc;
            }
        }
    }
#endif

    // call the UFuncton...
#if !SUPPORTS_RPC_CALL && !WITH_EDITOR
    if (FinalFunction == Function && FinalFunction->HasAnyFunctionFlags(FUNC_Native) && NumCalls == 1)
    {
        //FMemory::Memzero((uint8*)Params + FinalFunction->ParmsSize, FinalFunction->PropertiesSize - FinalFunction->ParmsSize);
        uint8* ReturnValueAddress = FinalFunction->ReturnValueOffset != MAX_uint16 ? (uint8*)Params + FinalFunction->ReturnValueOffset : nullptr;
        FFrame NewStack(Object, FinalFunction, Params, nullptr, GetChildProperties(Function));
        NewStack.OutParms = OutParmRec;
        FinalFunction->Invoke(Object, NewStack, ReturnValueAddress);
    }
    else
#endif
    {
        if (bRemote)
        {
            Object->CallRemoteFunction(FinalFunction, Params, nullptr, nullptr);
        }
        else
        {
            Object->UObject::ProcessEvent(FinalFunction, Params);
        }
    }

    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // push 'out' properties to Lua stack
    return NumReturnValues;
}

/**
 * Fire a delegate
 */
int32 FFunctionDesc::ExecuteDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FScriptDelegate *ScriptDelegate)
{
    if (!ScriptDelegate || !ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid FScriptDelegate, or FScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags);
    ScriptDelegate->ProcessDelegate<UObject>(Params);
    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);
    return NumReturnValues;
}

/**
 * Fire a multicast delegate
 */
void FFunctionDesc::BroadcastMulticastDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FMulticastScriptDelegate *ScriptDelegate)
{
    if (!ScriptDelegate || !ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid FMulticastScriptDelegate, or FMulticastScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags);
    ScriptDelegate->ProcessMulticastDelegate<UObject>(Params);
    PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // !!! have no return values for multi-cast delegates
}

/**
 * Prepare values of properties for the UFunction
 */
void* FFunctionDesc::PreCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, TArray<bool> &CleanupFlags, void *Userdata)
{
    void *Params = nullptr;
#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (NumCalls < 1)
    {
        Params = Buffer;
    }
    else
#endif
    Params = Function->ParmsSize > 0 ? FMemory::Malloc(Function->ParmsSize, 16) : nullptr;

    ++NumCalls;

    int32 ParamIndex = 0;
    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        FPropertyDesc *Property = Properties[i];
        Property->InitializeValue(Params);
        if (i == LatentPropertyIndex)
        {
            // bind a callback to the latent function
            int32 ThreadRef = *((int32*)Userdata);
            FLatentActionInfo LatentActionInfo(ThreadRef, GetTypeHash(FGuid::NewGuid()), TEXT("OnLatentActionCompleted"), (UObject*)GLuaCxt->GetManager());
            Property->CopyValue(Params, &LatentActionInfo);
            continue;
        }
        if (i == ReturnPropertyIndex)
        {
            CleanupFlags[i] = ParamIndex < NumParams ? !Property->CopyBack(L, FirstParamIndex + ParamIndex, Params) : true;
            continue;
        }
        if (ParamIndex < NumParams)
        {
            CleanupFlags[i] = Property->SetValue(L, Params, FirstParamIndex + ParamIndex, false);
        }
        else if (!Property->IsOutParameter())
        {
            if (DefaultParams)
            {
                // set value for default parameter
                IParamValue **DefaultValue = DefaultParams->Parameters.Find(Property->GetProperty()->GetFName());
                if (DefaultValue)
                {
                    const void *ValuePtr = (*DefaultValue)->GetValue();
                    Property->CopyValue(Params, ValuePtr);
                    CleanupFlags[i] = true;
                }
            }
        }
        ++ParamIndex;
    }

    return Params;
}

/**
 * Handling 'out' properties
 */
int32 FFunctionDesc::PostCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, void *Params, const TArray<bool> &CleanupFlags)
{
    int32 NumReturnValues = 0;

    for (int32 Index : OutPropertyIndices)
    {
        FPropertyDesc *Property = Properties[Index];
        if (Index >= NumParams || !Property->CopyBack(L, Params, FirstParamIndex + Index))
        {
            Property->GetValue(L, Params, true);
            ++NumReturnValues;
        }
    }

    if (ReturnPropertyIndex > INDEX_NONE)
    {
        FPropertyDesc *Property = Properties[ReturnPropertyIndex];
        if (!CleanupFlags[ReturnPropertyIndex])
        {
            int32 ReturnIndexInStack = FirstParamIndex + ReturnPropertyIndex;
            bool bResult = Property->CopyBack(L, Params, ReturnIndexInStack);
            check(bResult);
            lua_pushvalue(L, ReturnIndexInStack);
        }
        else
        {
            Property->GetValue(L, Params, true);
        }
        ++NumReturnValues;
    }

    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        if (CleanupFlags[i])
        {
            Properties[i]->DestroyValue(Params);
        }
    }

    --NumCalls;

#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (NumCalls > 0)
#endif
    FMemory::Free(Params);

    return NumReturnValues;
}

/**
 * Get OutParmRec for a non-const reference property
 */
static FOutParmRec* FindOutParmRec(FOutParmRec *OutParam, FProperty *OutProperty)
{
    while (OutParam)
    {
        if (OutParam->Property == OutProperty)
        {
            return OutParam;
        }
        OutParam = OutParam->NextOutParm;
    }
    return nullptr;
}

/**
 * Call Lua function that overrides this UFunction. 
 */
bool FFunctionDesc::CallLuaInternal(lua_State *L, void *InParams, FOutParmRec *OutParams, void *RetValueAddress) const
{
    // prepare parameters for Lua function
    FOutParmRec *OutParam = OutParams;
    for (const FPropertyDesc *Property : Properties)
    {
        if (Property->IsReturnParameter())
        {
            continue;
        }

        if (Property->IsConstOutParameter())
        {
            OutParam = FindOutParmRec(OutParam, Property->GetProperty());
            if (OutParam)
            {
                Property->GetValueInternal(L, OutParam->PropAddr, false);
                OutParam = OutParam->NextOutParm;
                continue;
            }
        }

        Property->GetValue(L, InParams, false);
    }

    int32 NumParams = HasReturnProperty() ? Properties.Num() : 1 + Properties.Num();
    int32 NumResult = GetNumOutProperties();
    bool bSuccess = CallFunction(L, NumParams, NumResult);      // pcall
    if (!bSuccess)
    {
        return false;
    }

    int32 OutPropertyIndex = -NumResult;
    OutParam = OutParams;
    for (int32 i = 0; i < OutPropertyIndices.Num(); ++i)
    {
        FPropertyDesc *OutProperty = Properties[OutPropertyIndices[i]];
        OutParam = FindOutParmRec(OutParam, OutProperty->GetProperty());
        check(OutParam);
        int32 Type = lua_type(L, OutPropertyIndex);
        if (Type == LUA_TNIL)
        {
            bSuccess = OutProperty->CopyBack(OutParam->PropAddr, OutProperty->GetProperty()->ContainerPtrToValuePtr<void>(InParams));   // copy back value to out property
            if (!bSuccess)
            {
                UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("Can't copy value back!"));
            }
        }
        else
        {
            OutProperty->SetValueInternal(L, OutParam->PropAddr, OutPropertyIndex, true);       // set value for out property
        }
        OutParam = OutParam->NextOutParm;
        ++OutPropertyIndex;
    }
    if (ReturnPropertyIndex > INDEX_NONE)
    {
        check(RetValueAddress);
        Properties[ReturnPropertyIndex]->SetValueInternal(L, RetValueAddress, -1, true);        // set value for return property
    }

    lua_pop(L, NumResult);
    return true;
}
