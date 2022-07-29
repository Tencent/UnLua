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
#include "LuaCore.h"
#include "DefaultParamCollection.h"
#include "LowLevel.h"
#include "LuaFunction.h"
#include "UnLua.h"
#include "UnLuaDebugBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LuaDeadLoopCheck.h"

/**
 * Function descriptor constructor
 */
FFunctionDesc::FFunctionDesc(UFunction *InFunction, FParameterCollection *InDefaultParams)
    : DefaultParams(InDefaultParams), ReturnPropertyIndex(INDEX_NONE), LatentPropertyIndex(INDEX_NONE)
    , NumRefProperties(0), NumCalls(0), bStaticFunc(false), bInterfaceFunc(false)
{
    check(InFunction);

    Function = InFunction;
    FuncName = InFunction->GetName();
    ParmsSize = InFunction->ParmsSize;

#if SUPPORTS_RPC_CALL
    if (InFunction->HasAnyFunctionFlags(FUNC_Net))
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FString::Printf(TEXT("%s_RPC"), *FuncName));
    else
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FuncName);
#else
    LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FuncName);
#endif
    
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
        FMemory::Memzero(Buffer, InFunction->ParmsSize);
        UNLUA_STAT_MEMORY_ALLOC(Buffer, Lua)
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
        FPropertyDesc* PropertyDesc = FPropertyDesc::Create(Property);
        int32 Index = Properties.Add(TUniquePtr<FPropertyDesc>(PropertyDesc));
        if (PropertyDesc->IsReturnParameter())
        {
            ReturnPropertyIndex = Index;                                // return property
        }
        else if (LatentPropertyIndex == INDEX_NONE && Property->GetFName() == NAME_LatentInfo)
        {
            LatentPropertyIndex = Index;                                // 'LatentInfo' property for latent function
        }
        else if (Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
        {
            ++NumRefProperties;

            // pre-create OutParmRec for 'out' property
#if !SUPPORTS_RPC_CALL
            FOutParmRec *Out = (FOutParmRec*)FMemory::Malloc(sizeof(FOutParmRec), alignof(FOutParmRec));
            UNLUA_STAT_MEMORY_ALLOC(Out, OutParmRec);
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
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("~FFunctionDesc : %s,%p"), *FuncName, this);
#endif

    // free persistent parameter buffer
#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (Buffer)
    {
        UNLUA_STAT_MEMORY_FREE(Buffer, PersistentParamBuffer);
        FMemory::Free(Buffer);
    }
#endif

    // free pre-created OutParmRec
#if !SUPPORTS_RPC_CALL
    while (OutParmRec)
    {
        FOutParmRec *NextOut = OutParmRec->NextOutParm;
        UNLUA_STAT_MEMORY_FREE(OutParmRec, OutParmRec);
        FMemory::Free(OutParmRec);
        OutParmRec = NextOut;
    }
#endif
}


void FFunctionDesc::CallLua(lua_State* L, lua_Integer FunctionRef, lua_Integer SelfRef, FFrame& Stack, RESULT_DECL)
{
    lua_pushcfunction(L, UnLua::ReportLuaCallError);
    check(Function.IsValid());
    lua_rawgeti(L, LUA_REGISTRYINDEX, FunctionRef);
    check(lua_isfunction(L, -1));
    lua_rawgeti(L, LUA_REGISTRYINDEX, SelfRef);
    check(lua_istable(L, -1));

    void* InParms = nullptr;
    FOutParmRec* OutParms = Stack.OutParms;
    const bool bUnpackParams = Stack.CurrentNativeFunction && Stack.Node != Stack.CurrentNativeFunction;
    if (bUnpackParams)
    {
#if ENABLE_PERSISTENT_PARAM_BUFFER
        InParms = Buffer;
#endif
        if (!InParms)
            InParms = ParmsSize > 0 ? FMemory::Malloc(ParmsSize, 16) : nullptr;

        FOutParmRec* FirstOut = nullptr;
        FOutParmRec* LastOut = nullptr;

        for (FProperty* Property = (FProperty*)(Function->ChildProperties);
             *Stack.Code != EX_EndFunctionParms;
             Property = (FProperty*)(Property->Next))
        {
            if (Property->PropertyFlags & CPF_OutParm)
            {
                Stack.Step(Stack.Object, Property->ContainerPtrToValuePtr<uint8>(InParms));

                CA_SUPPRESS(6263)
                FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
                ensure(Stack.MostRecentPropertyAddress);
                Out->PropAddr = (Stack.MostRecentPropertyAddress != nullptr) ? Stack.MostRecentPropertyAddress : Property->ContainerPtrToValuePtr<uint8>(InParms);
                Out->Property = Property;

                if (FirstOut)
                {
                    LastOut->NextOutParm = Out;
                    LastOut = Out;    
                }
                else
                {
                    FirstOut = Out;
                    LastOut = Out;
                }
            }
            else
            {
                Stack.Step(Stack.Object, Property->ContainerPtrToValuePtr<uint8>(InParms));
            }
        }

        if (LastOut)
        {
            LastOut->NextOutParm = Stack.OutParms;
            OutParms = FirstOut;
        }

        check(Stack.PeekCode() == EX_EndFunctionParms);
        Stack.SkipCode(1); // skip EX_EndFunctionParms
    }
    else
    {
        InParms = Stack.Locals;
    }

    CallLuaInternal(L, InParms , OutParms, RESULT_PARAM);

#if !ENABLE_PERSISTENT_PARAM_BUFFER
    if (bUnpackParams && InParms)
        FMemory::Free(InParms);
#endif
}

bool FFunctionDesc::CallLua(lua_State* L, int32 LuaRef, void* Params, UObject* Self)
{
    bool bOk = PushFunction(L, Self, LuaRef);
    if (!bOk)
        return false;

    const bool bHasReturnParam = Function->ReturnValueOffset != MAX_uint16;
    uint8* ReturnValueAddress = bHasReturnParam ? ((uint8*)Params + Function->ReturnValueOffset) : nullptr;
    bOk = CallLuaInternal(L, Params, nullptr, ReturnValueAddress);
    return bOk;
}

/**
 * Call the UFunction
 */
int32 FFunctionDesc::CallUE(lua_State *L, int32 NumParams, void *Userdata)
{
    check(Function.IsValid());

    // !!!Fix!!!
    // when static function passed an object, it should be ignored auto
    int32 FirstParamIndex = 1;
    UObject *Object = nullptr;
    if (bStaticFunc)
    {
        UClass *OuterClass = Function->GetOuterUClass();
        Object = OuterClass->GetDefaultObject();                // get CDO for static function
    }
    else if (NumParams > 0)
    {
        Object = UnLua::GetUObject(L, 1, false);
        ++FirstParamIndex;
        --NumParams;
    }

    if (Object == UnLua::LowLevel::ReleasedPtr)
    {
        luaL_error(L, "attempt to call UFunction '%s' on released object.", TCHAR_TO_UTF8(*FuncName));
        return 0;
    }

    if (Object == nullptr)
    {
        luaL_error(L, "attempt to call UFunction '%s' on NULL object. (check the usage of ':' and '.')", TCHAR_TO_UTF8(*FuncName));
        return 0;
    }

#if SUPPORTS_RPC_CALL
    int32 Callspace = Object->GetFunctionCallspace(Function.Get(), nullptr);
    bool bRemote = Callspace & FunctionCallspace::Remote;
    bool bLocal = Callspace & FunctionCallspace::Local;
#else
    bool bRemote = false;
    bool bLocal = true;
#endif

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Userdata);      // prepare values of properties

    UFunction *FinalFunction = Function.Get();
    if (bInterfaceFunc)
    {
        // get target UFunction if it's a function in Interface
        FName FunctionName = Function->GetFName();
        FinalFunction = Object->GetClass()->FindFunctionByName(FunctionName);
        if (!FinalFunction)
        {
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("ERROR! Can't find UFunction '%s' in target object!"), *FuncName);

#if !ENABLE_PERSISTENT_PARAM_BUFFER
            if (Params)
                FMemory::Free(Params);
#endif

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
    {
        if (!Function->HasAnyFunctionFlags(FUNC_Net))
        {
            const auto LuaFunction = Cast<ULuaFunction>(Function);
            const auto Overridden = LuaFunction == nullptr ? nullptr : LuaFunction->GetOverridden();
            if (Overridden)
                FinalFunction = Overridden;
        }
    }
#endif

    // call the UFuncton...
#if !SUPPORTS_RPC_CALL
    if (FinalFunction == Function && FinalFunction->HasAnyFunctionFlags(FUNC_Native) && NumCalls == 1)
    {
        //FMemory::Memzero((uint8*)Params + FinalFunction->ParmsSize, FinalFunction->PropertiesSize - FinalFunction->ParmsSize);
        uint8* ReturnValueAddress = FinalFunction->ReturnValueOffset != MAX_uint16 ? (uint8*)Params + FinalFunction->ReturnValueOffset : nullptr;
        FMemory::Memcpy(Buffer, Params, Function->ParmsSize);
        FFrame NewStack(Object, FinalFunction, Params, nullptr, GetChildProperties(Function));
        NewStack.OutParms = OutParmRec;
        FinalFunction->Invoke(Object, NewStack, ReturnValueAddress);
    }
    else
#endif
    {   
        // Func_NetMuticast both remote and local
        // local automatic checked remote and local,so local first
        if (bLocal)
        {   
            Object->UObject::ProcessEvent(FinalFunction, Params);
        }
        if (bRemote && !bLocal)
        {
            Object->CallRemoteFunction(FinalFunction, Params, nullptr, nullptr);
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
#if ENABLE_PERSISTENT_PARAM_BUFFER
    void* Params = Buffer;
#else
    void* Params = Function->ParmsSize > 0 ? FMemory::Malloc(Function->ParmsSize, 16) : nullptr;
#endif

    ++NumCalls;

    int32 ParamIndex = 0;
    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        const auto& Property = Properties[i];
        Property->InitializeValue(Params);
        if (i == LatentPropertyIndex)
        {
            const int32 ThreadRef = *((int32*)Userdata);
            void* ContainerPtr = (uint8*)Params;// + Property->GetOffset();
            if(lua_type(L, FirstParamIndex + ParamIndex) == LUA_TUSERDATA)
            {
                // custom latent action info
                FLatentActionInfo Info = UnLua::Get<FLatentActionInfo>(L, FirstParamIndex + ParamIndex, UnLua::TType<FLatentActionInfo>());
                Property->CopyValue(ContainerPtr, &Info);
                continue;
            }

            // bind a callback to the latent function
            auto& Env = UnLua::FLuaEnv::FindEnvChecked(L);
            FLatentActionInfo LatentActionInfo(ThreadRef, GetTypeHash(FGuid::NewGuid()), TEXT("OnLatentActionCompleted"), (Env.GetManager()));
            Property->CopyValue(ContainerPtr, &LatentActionInfo);
            continue;
        }
        if (i == ReturnPropertyIndex)
        {
            CleanupFlags[i] = ParamIndex < NumParams ? !Property->CopyBack(L, FirstParamIndex + ParamIndex, Params) : true;
            continue;
        }
        if (ParamIndex < NumParams)
        {   
#if ENABLE_TYPE_CHECK == 1
            FString ErrorMsg = "";
            if (!Property->CheckPropertyType(L, FirstParamIndex + ParamIndex, ErrorMsg))
            {
                UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("Invalid parameter type calling ufunction : %s,parameter : %d, error msg : %s"), *FuncName, ParamIndex, *ErrorMsg);
            }
#endif
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
            else
            {
#if ENABLE_TYPE_CHECK == 1
                FString ErrorMsg = "";
                if (!Property->CheckPropertyType(L, FirstParamIndex + ParamIndex, ErrorMsg))
                {
                    UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("Invalid parameter type calling ufunction : %s,parameter : %d, error msg : %s"), *FuncName, ParamIndex, *ErrorMsg);
                }
#endif
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

#if UNLUA_LEGACY_RETURN_ORDER
    for (int32 Index : OutPropertyIndices)
    {
        const auto& Property = Properties[Index];
        if (Index >= NumParams || !Property->CopyBack(L, Params, FirstParamIndex + Index))
        {
            Property->GetValue(L, Params, true);
            ++NumReturnValues;
        }
    }
#endif

    if (ReturnPropertyIndex > INDEX_NONE)
    {
        const auto& Property = Properties[ReturnPropertyIndex];
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

#if !UNLUA_LEGACY_RETURN_ORDER
    // !!!Fix!!!
    // out parameters always use return format, copyback is better,but some parameters such 
    // as int can not be copy back
    // c++ may has return and out params, we must push it on stack
    for (int32 Index : OutPropertyIndices)
    {
        const auto& Property = Properties[Index];
        if (Index >= NumParams || !Property->CopyBack(L, Params, FirstParamIndex + Index))
        {
            Property->GetValue(L, Params, true);
            ++NumReturnValues;
        }
    }
#endif

    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        if (CleanupFlags[i])
        {
            Properties[i]->DestroyValue(Params);
        }
    }

    --NumCalls;

#if !ENABLE_PERSISTENT_PARAM_BUFFER
    if (Params)
        FMemory::Free(Params);
#endif	

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
    for (const auto& Property : Properties)
    {
        if (Property->IsReturnParameter())
        {
            continue;
        }

        Property->GetValue(L, InParams, false);
    }

    // object is also pushed, return is push when return
    int32 NumParams = Properties.Num();
    int32 NumResult = OutPropertyIndices.Num();
    if (ReturnPropertyIndex == INDEX_NONE)
    {
        NumParams++;
    }
    else
    {
        NumResult++;
    }

    const auto& Env = UnLua::FLuaEnv::FindEnvChecked(L);
    const auto Guard = Env.GetDeadLoopCheck()->MakeGuard();
    bool bSuccess = CallFunction(L, NumParams, NumResult);      // pcall
    if (!bSuccess)
    {
        return false;
    }

    // out value
    // suppose out param is also pushed on stack? this is assumed done by user... so we can not trust it
    int32 NumResultOnStack = lua_gettop(L);
    if (NumResult <= NumResultOnStack)
    {
        int32 OutPropertyIndex = -NumResult;
#if !UNLUA_LEGACY_RETURN_ORDER
        if (ReturnPropertyIndex > INDEX_NONE)
            OutPropertyIndex++;
#endif

        OutParam = OutParams;

        for (int32 i = 0; i < OutPropertyIndices.Num(); ++i)
        {
            const auto& OutProperty = Properties[OutPropertyIndices[i]];
            if (OutProperty->IsReferenceParameter())
            {
                continue;
            }
            OutParam = FindOutParmRec(OutParam, OutProperty->GetProperty());
            if (!OutParam)
            {
                OutProperty->SetValue(L, InParams, OutPropertyIndex, true);
            }
            else
            {
                // user do push it on stack?
                int32 Type = lua_type(L, OutPropertyIndex);
                if (Type == LUA_TNIL)
                {
                    // so we need copy it back from input parameter
                    OutProperty->CopyBack(OutParam->PropAddr, OutProperty->GetProperty()->ContainerPtrToValuePtr<void>(InParams));   // copy back value to out property
                }
                else
                {   
                    // copy it from stack
                    OutProperty->SetValueInternal(L, OutParam->PropAddr, OutPropertyIndex, true);       // set value for out property
                }
                OutParam = OutParam->NextOutParm;
            }
            ++OutPropertyIndex;
        }
    }
    
    // return value
    if (ReturnPropertyIndex > INDEX_NONE)
    {   
        if (NumResultOnStack < 1)
        {
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("FuncName %s has return value, but no value found on stack!"),*FuncName);
        }
        else
        {
            const auto& ReturnProperty = Properties[ReturnPropertyIndex];

#if UNLUA_LEGACY_RETURN_ORDER
            constexpr auto IndexInStack = -1;
#else
            const auto IndexInStack = -NumResult;
#endif
            
            // set value for blueprint side return property
            const FOutParmRec* RetParam = OutParam ? FindOutParmRec(OutParam, ReturnProperty->GetProperty()) : nullptr;
            if (RetParam)
                ReturnProperty->SetValueInternal(L, RetParam->PropAddr, IndexInStack, true);

            // set value for return property
            check(RetValueAddress);
            ReturnProperty->SetValueInternal(L, RetValueAddress, IndexInStack, true);
        }
    }

    lua_pop(L, NumResult);
    return true;
}

