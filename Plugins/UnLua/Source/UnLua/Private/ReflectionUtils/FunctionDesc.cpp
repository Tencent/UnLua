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
#include "Containers/StaticBitArray.h"

/**
 * Function descriptor constructor
 */
FFunctionDesc::FFunctionDesc(UFunction *InFunction, FParameterCollection *InDefaultParams)
    : DefaultParams(InDefaultParams), ReturnPropertyIndex(INDEX_NONE), LatentPropertyIndex(INDEX_NONE)
    , bStaticFunc(false), bInterfaceFunc(false)
{
    check(InFunction);

    Function = InFunction;
    FuncName = InFunction->GetName();
    ParmsSize = InFunction->ParmsSize;

    if (InFunction->HasAnyFunctionFlags(FUNC_Net))
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FString::Printf(TEXT("%s_RPC"), *FuncName));
    else
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FuncName);

    bStaticFunc = InFunction->HasAnyFunctionFlags(FUNC_Static);

    const auto OuterClass = Cast<UClass>(InFunction->GetOuter());
    bInterfaceFunc = OuterClass && OuterClass->HasAnyClassFlags(CLASS_Interface) && OuterClass != UInterface::StaticClass();

    Buffer = FParamBufferFactory::Get(*InFunction);

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
            if (!Property->HasAnyPropertyFlags(CPF_ConstParm))
            {
                OutPropertyIndices.Add(Index);                          // non-const reference property
            }
        }
    }
}

void FFunctionDesc::CallLua(lua_State* L, lua_Integer FunctionRef, lua_Integer SelfRef, FFrame& Stack, RESULT_DECL)
{
#if ENABLE_UNREAL_INSIGHTS && CPUPROFILERTRACE_ENABLED
    TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*FuncName);
#endif
    
    lua_pushcfunction(L, UnLua::ReportLuaCallError);
    check(Function.IsValid());
    lua_rawgeti(L, LUA_REGISTRYINDEX, FunctionRef);
    check(lua_isfunction(L, -1));
    lua_rawgeti(L, LUA_REGISTRYINDEX, SelfRef);
    check(lua_istable(L, -1));

    void* InParms;
    FOutParmRec* OutParms = Stack.OutParms;
    const bool bUnpackParams = Stack.CurrentNativeFunction && Stack.Node != Stack.CurrentNativeFunction;
    if (bUnpackParams)
    {
        InParms = Buffer->Get();

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

    if (bUnpackParams && InParms)
        Buffer->Pop(InParms);
}

bool FFunctionDesc::CallLua(lua_State* L, int32 LuaRef, void* Params, UObject* Self)
{
#if ENABLE_UNREAL_INSIGHTS && CPUPROFILERTRACE_ENABLED
    TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*FuncName);
#endif
    
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
#if ENABLE_UNREAL_INSIGHTS && CPUPROFILERTRACE_ENABLED
    TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*FuncName);
#endif

    check(Function.IsValid());

    UObject* Object;
    int32 FirstParamIndex;
    if (bStaticFunc)
    {
        Object = Function->GetOuterUClass()->GetDefaultObject();                // get CDO for static function
        FirstParamIndex = 1;
    }
    else if (NumParams > 0)
    {
        Object = UnLua::GetUObject(L, 1, false);
        FirstParamIndex = 2;
        --NumParams;
    }
    else
    {
        Object = nullptr;
        FirstParamIndex = 1;
    }

    FString Error;
    if (UNLIKELY(!CheckObject(Object, Error)))
        return luaL_error(L, TCHAR_TO_UTF8(*Error));

    int32 Callspace = Object->GetFunctionCallspace(Function.Get(), nullptr);
    bool bRemote = Callspace & FunctionCallspace::Remote;
    bool bLocal = Callspace & FunctionCallspace::Local;

    FFlagArray CleanupFlags;
    const auto Params = Buffer->Get(); 
    PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Params, Userdata);      // prepare values of properties
    auto FinalFunction = bInterfaceFunc
                             ? Object->GetClass()->FindFunctionByName(Function->GetFName())
                             : Function.Get();

#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    if (!Function->HasAnyFunctionFlags(FUNC_Net))
    {
        const auto LuaFunction = ULuaFunction::Get(Function.Get());
        if (LuaFunction && LuaFunction->GetOverridden())
            FinalFunction = LuaFunction->GetOverridden();
    }
#endif

    // call the UFuncton...
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

    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // push 'out' properties to Lua stack
    Buffer->Pop(Params);
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

    FFlagArray CleanupFlags;
    const auto Params = Buffer->Get();
    PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Params);
    ScriptDelegate->ProcessDelegate<UObject>(Params);
    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);
    Buffer->Pop(Params);
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

    FFlagArray CleanupFlags;
    const auto Params = Buffer->Get();
    PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Params);
    ScriptDelegate->ProcessMulticastDelegate<UObject>(Params);
    PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // !!! have no return values for multi-cast delegates
    Buffer->Pop(Params);
}

/**
 * Prepare values of properties for the UFunction
 */
void FFunctionDesc::PreCall(lua_State* L, int32 NumParams, int32 FirstParamIndex, FFlagArray& CleanupFlags, void* Params, void* Userdata)
{
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
            CleanupFlags[i] = ParamIndex >= NumParams || !Property->CopyBack(L, FirstParamIndex + ParamIndex, Params);
            continue;
        }
        if (ParamIndex < NumParams)
        {   
#if ENABLE_TYPE_CHECK == 1
            FString ErrorMsg = "";
            if (Property->CheckPropertyType(L, FirstParamIndex + ParamIndex, ErrorMsg))
                CleanupFlags[i] = Property->WriteValue_InContainer(L, Params, FirstParamIndex + ParamIndex, false);
            else
                UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("Invalid parameter type calling ufunction : %s,parameter : %d, error msg : %s"), *FuncName, ParamIndex, *ErrorMsg);
#else
            CleanupFlags[i] = Property->WriteValue_InContainer(L, Params, FirstParamIndex + ParamIndex, false);
#endif
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
}

/**
 * Handling 'out' properties
 */
int32 FFunctionDesc::PostCall(lua_State * L, int32 NumParams, int32 FirstParamIndex, void* Params, const FFlagArray& CleanupFlags)
{
    int32 NumReturnValues = 0;

#if UNLUA_LEGACY_RETURN_ORDER
    for (int32 Index : OutPropertyIndices)
    {
        const auto& Property = Properties[Index];
        if (Index >= NumParams || !Property->CopyBack(L, Params, FirstParamIndex + Index))
        {
            Property->ReadValue_InContainer(L, Params, true);
            ++NumReturnValues;
        }
    }
#endif

    if (ReturnPropertyIndex > INDEX_NONE)
    {
        const auto& Property = Properties[ReturnPropertyIndex];
        if (CleanupFlags[ReturnPropertyIndex])
        {
            Property->ReadValue_InContainer(L, Params, true);
        }
        else
        {
            int32 ReturnIndexInStack = FirstParamIndex + ReturnPropertyIndex;
            bool bResult = Property->CopyBack(L, Params, ReturnIndexInStack);
            check(bResult);
            lua_pushvalue(L, ReturnIndexInStack);
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
            Property->ReadValue_InContainer(L, Params, true);
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
    // -1 = [table/userdata] UObject for self
    // -2 = [function] to call
    // -3 = [function] ReportLuaCallError
    const auto ErrorHandlerIndex = lua_gettop(L) - 2;

    const auto& Env = UnLua::FLuaEnv::FindEnvChecked(L);
    const auto DanglingGuard = Env.GetDanglingCheck()->MakeGuard();

    if (InParams)
    {
        // prepare parameters for Lua function
        for (const auto& Property : Properties)
        {
            if (Property->IsReturnParameter())
                continue;

            Property->ReadValue_InContainer(L, InParams, !UNLUA_LEGACY_ARGS_PASSING);
        }
    }

    // object is also pushed, return is push when return
    int32 NumParams = Properties.Num();
    if (ReturnPropertyIndex == INDEX_NONE)
        NumParams++;

    const auto Guard = Env.GetDeadLoopCheck()->MakeGuard();
    if (lua_pcall(L, NumParams, LUA_MULTRET, -(NumParams + 2)) != LUA_OK)
    {
        lua_settop(L, ErrorHandlerIndex - 1);
        return false;
    }

    // out value
    // suppose out param is also pushed on stack? this is assumed done by user... so we can not trust it
    int32 NumResultOnStack = lua_gettop(L) - ErrorHandlerIndex;
    int32 OutPropertyIndex = ErrorHandlerIndex + 1;
#if !UNLUA_LEGACY_RETURN_ORDER
    if (ReturnPropertyIndex > INDEX_NONE)
        OutPropertyIndex++;
#endif

    FOutParmRec *OutParam = OutParams;
    for (int32 i = 0; i < OutPropertyIndices.Num(); i++)
    {
        const auto& OutProperty = Properties[OutPropertyIndices[i]];
        if (OutProperty->IsReferenceParameter())
            continue;

        OutParam = FindOutParmRec(OutParam, OutProperty->GetProperty());
        if (OutParam)
        {
            // user do push it on stack?
            if (OutPropertyIndex - ErrorHandlerIndex > NumResultOnStack)
            {
                // so we need copy it back from input parameter
                OutProperty->CopyBack(OutParam->PropAddr, OutProperty->GetProperty()->ContainerPtrToValuePtr<void>(InParams)); // copy back value to out property
            }
            else
            {
                // copy it from stack
                OutProperty->WriteValue(L, OutParam->PropAddr, OutPropertyIndex, true); // set value for out property
            }
            OutParam = OutParam->NextOutParm;
        }
        else
        {
            OutProperty->WriteValue_InContainer(L, InParams, OutPropertyIndex, true);
        }
        OutPropertyIndex++;
    }

    // return value
    if (ReturnPropertyIndex > INDEX_NONE)
    {
        const auto& ReturnProperty = Properties[ReturnPropertyIndex];
        if (NumResultOnStack < 1)
        {
            ReturnProperty->GetProperty()->InitializeValue(RetValueAddress);
#if ENABLE_TYPE_CHECK == 1
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("FuncName %s has return value, but no value found on stack!"),*FuncName);
#endif
        }
        else
        {
#if UNLUA_LEGACY_RETURN_ORDER
            constexpr auto IndexInStack = -1;
#else
            const auto IndexInStack = -NumResultOnStack;
#endif
            ReturnProperty->WriteValue(L, RetValueAddress, IndexInStack, true);
        }
    }

    lua_settop(L, ErrorHandlerIndex - 1);
    return true;
}

bool FFunctionDesc::CheckObject(UObject* Object, FString& Error) const
{
    if (Object == UnLua::LowLevel::ReleasedPtr)
    {
        Error = FString::Printf(TEXT("attempt to call UFunction '%s' on released object."), *FuncName);
        return false;
    }

    if (Object == nullptr)
    {
        Error = FString::Printf(TEXT("attempt to call UFunction '%s' on NULL object. (check the usage of ':' and '.')"), *FuncName);
        return false;
    }

    if (Object->IsUnreachable())
    {
        Error = FString::Printf(TEXT("attempt to call UFunction '%s' on Unreachable object '%s'."), *FuncName, *Object->GetName());
        return false;
    }

    if (Object->HasAnyFlags(RF_NeedInitialization))
    {
        Error = FString::Printf(TEXT("attempt to call UFunction '%s' in lua Initialize function on object '%s'."), *FuncName, *Object->GetName());
        return false;
    }

#if ENABLE_TYPE_CHECK && WITH_EDITOR
    const auto LuaFunction = ULuaFunction::Get(Function.Get());
    const auto TargetClass = LuaFunction && LuaFunction->GetOverridden()
                           ? LuaFunction->GetOverriddenUClass()
                           : Function->GetOwnerClass();

    if (bInterfaceFunc)
    {
        if (Object->GetClass()->ImplementsInterface(TargetClass))
            return true;
    }
    else
    {
        if (Object->IsA(TargetClass))
            return true;
    }

    Error = FString::Printf(TEXT("attempt to call UFunction '%s' on invalid self type. '%s' required but got '%s'."),
                            *FuncName, *TargetClass->GetName(), *Object->GetClass()->GetName());
    return false;
#else
    return true;
#endif
}
