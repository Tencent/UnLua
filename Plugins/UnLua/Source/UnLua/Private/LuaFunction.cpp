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


#include "LuaFunction.h"
#include "LuaCore.h"
#include "UnLuaModule.h"
#include "ReflectionUtils/PropertyDesc.h"

DEFINE_FUNCTION(ULuaFunction::execCallLua)
{
    const auto LuaFunction = Cast<ULuaFunction>(Stack.CurrentNativeFunction);
    LuaFunction->Call(Context, Stack, RESULT_PARAM);
}

bool ULuaFunction::IsOverridable(const UFunction* Function)
{
    static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
    static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
    return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

bool ULuaFunction::Override(UFunction* Function, UClass* Outer, UnLua::FLuaEnv* LuaEnv, FName NewName)
{
    ULuaFunction* LuaFunction;
    const auto bReplace = Function->GetOuter() == Outer;
    if (bReplace)
    {
        // ReplaceFunction
        LuaFunction = Cast<ULuaFunction>(Function);
        if (LuaFunction)
        {
            LuaFunction->Initialize();
            return true;
        }

        const auto OverriddenName = FString::Printf(TEXT("%s%s"), *Function->GetName(), TEXT("__Overridden"));
        constexpr auto RenameFlags = REN_DontCreateRedirectors | REN_DoNotDirty | REN_ForceNoResetLoaders | REN_NonTransactional;
        Function->Rename(*OverriddenName, Function->GetOuter(), RenameFlags);
    }
    else
    {
        // AddFunction
        if (Outer->FindFunctionByName(NewName, EIncludeSuperFlag::ExcludeSuper))
            return false;

        if (Function->HasAnyFunctionFlags(FUNC_Native))
        {
            // Need to do this before the call to DuplicateObject in the case that the super-function already has FUNC_Native
            Outer->AddNativeFunction(*NewName.ToString(), &execCallLua);
        }
    }

    FObjectDuplicationParameters DuplicationParams(Function, Outer);
    DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
    DuplicationParams.DestName = NewName;
    DuplicationParams.DestClass = StaticClass();
    LuaFunction = Cast<ULuaFunction>(StaticDuplicateObjectEx(DuplicationParams));
    LuaFunction->FunctionFlags |= FUNC_Native;
    LuaFunction->Overridden = Function;
    LuaFunction->ClearInternalFlags(EInternalObjectFlags::Native);
    LuaFunction->SetNativeFunc(execCallLua);

    if (bReplace)
        LuaFunction->SetSuperStruct(Function->GetSuperStruct());
    else
        LuaFunction->SetSuperStruct(Function);

    if (!FPlatformProperties::RequiresCookedData())
        UMetaData::CopyMetadata(Function, LuaFunction);

    LuaFunction->StaticLink(true);
    LuaFunction->Initialize();

    Outer->AddFunctionToFunctionMap(LuaFunction, NewName);

    if (Outer->IsRooted() || GUObjectArray.IsDisregardForGC(Outer))
    {
        LuaFunction->AddToRoot();
    }
    else
    {
        LuaFunction->Next = Outer->Children;
        Outer->Children = LuaFunction;
        LuaFunction->AddToCluster(Outer);
    }

    return true;
}

void ULuaFunction::GetOverridableFunctions(UClass* Class, TMap<FName, UFunction*>& Functions)
{
    if (!Class)
        return;

    // all 'BlueprintEvent'
    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::IncludeInterfaces); It; ++It)
    {
        UFunction* Function = *It;
        if (!IsOverridable(Function))
            continue;
        FName FuncName = Function->GetFName();
        UFunction** FuncPtr = Functions.Find(FuncName);
        if (!FuncPtr)
            Functions.Add(FuncName, Function);
    }

    // all 'RepNotifyFunc'
    for (int32 i = 0; i < Class->ClassReps.Num(); ++i)
    {
        FProperty* Property = Class->ClassReps[i].Property;
        if (!Property->HasAnyPropertyFlags(CPF_RepNotify))
            continue;
        UFunction* Function = Class->FindFunctionByName(Property->RepNotifyFunc);
        if (!Function)
            continue;
        UFunction** FuncPtr = Functions.Find(Property->RepNotifyFunc);
        if (!FuncPtr)
            Functions.Add(Property->RepNotifyFunc, Function);
    }
}

void ULuaFunction::Initialize()
{
    ReturnPropertyIndex = INDEX_NONE;
    LatentPropertyIndex = INDEX_NONE;
    NumRefProperties = 0;
    NumCalls = 0;
    bInterfaceFunc = false;
    Properties.Reset();
    OutPropertyIndices.Reset();

    bStaticFunc = this->HasAnyFunctionFlags(FUNC_Static); // a static function?
    UClass* OuterClass = this->GetOuterUClass();
    if (OuterClass->HasAnyClassFlags(CLASS_Interface) && OuterClass != UInterface::StaticClass())
        bInterfaceFunc = true; // a function in interface?

    bHasDelegateParams = false;
    // create persistent parameter buffer. memory for speed
#if ENABLE_PERSISTENT_PARAM_BUFFER
    Buffer = nullptr;
    if (this->ParmsSize > 0)
    {
        Buffer = FMemory::Malloc(this->ParmsSize, 16);
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
    Properties.Reserve(this->NumParms);
    for (TFieldIterator<FProperty> It(this); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        FProperty* Property = *It;
        FPropertyDesc* PropertyDesc = FPropertyDesc::Create(Property);
        int32 Index = Properties.Add(PropertyDesc);
        if (PropertyDesc->IsReturnParameter())
        {
            ReturnPropertyIndex = Index; // return property
        }
        else if (LatentPropertyIndex == INDEX_NONE && Property->GetFName() == NAME_LatentInfo)
        {
            LatentPropertyIndex = Index; // 'LatentInfo' property for latent function
        }
        else if (Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
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
                OutPropertyIndices.Add(Index); // non-const reference property
            }
        }

        if (!bHasDelegateParams && !PropertyDesc->IsReturnParameter())
        {
            int8 PropertyType = PropertyDesc->GetPropertyType();
            if (PropertyType == CPT_Delegate
                || PropertyType == CPT_MulticastDelegate
                || PropertyType == CPT_MulticastSparseDelegate)
            {
                bHasDelegateParams = true;
            }
        }
    }

#if SUPPORTS_RPC_CALL
    if (HasAnyFunctionFlags(FUNC_Net))
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*FString::Printf(TEXT("%s_RPC"), *GetName()));
    else
        LuaFunctionName = MakeUnique<FTCHARToUTF8>(*GetName());
#else
    LuaFunctionName = MakeUnique<FTCHARToUTF8>(*GetName());
    if (CurrentOutParmRec)
        CurrentOutParmRec->NextOutParm = nullptr;
#endif
}

UFunction* ULuaFunction::GetOverridden() const
{
    return Overridden.Get();
}

void ULuaFunction::Bind()
{
#if WITH_EDITOR
    const auto Outer = GetOuter();
    if (Outer && Outer->GetName().StartsWith("REINST_"))
    {
        FunctionFlags &= ~FUNC_Native;
        return;
    }
#endif
    Super::Bind();
}

void ULuaFunction::Call(UObject* Context, FFrame& Stack, void* const RetValueAddress)
{
    const auto Env = IUnLuaModule::Get().GetEnv(Context);
    if (!Env)
    {
        // PIE 结束时可能已经没有Lua环境了
        return;
    }

    // UE_LOG(LogTemp, Log, TEXT("Calling %s.%s from %s"), *Context->GetName(), *GetName(), *Env->GetName());

    const auto L = Env->GetMainState();
    if (!Env->GetObjectRegistry()->IsBound(Context))
        Env->TryBind(Context);

    // TODO: Refactor for performance
    bool bSuccess = PushFunction(L, Context, LuaFunctionName->Get()) != LUA_NOREF;
    if (!bSuccess)
        return;

    const bool bUnpackParams = Stack.CurrentNativeFunction && Stack.Node != Stack.CurrentNativeFunction;

    if (bUnpackParams)
    {
        void* Params = nullptr;
#if ENABLE_PERSISTENT_PARAM_BUFFER
        if (!bHasDelegateParams)
        {
            Params = Buffer;
        }
#endif
        if (!Params)
        {
            Params = ParmsSize > 0 ? FMemory::Malloc(ParmsSize, 16) : nullptr;
        }

        SkipCodes(Stack, Params);

        bSuccess = CallLuaInternal(L, Params, Stack.OutParms, RetValueAddress); // call Lua function...

        if (Params)
        {
#if ENABLE_PERSISTENT_PARAM_BUFFER
            if (bHasDelegateParams)
#endif
                FMemory::Free(Params);
        }
    }
    else
    {
        bSuccess = CallLuaInternal(L, Stack.Locals, Stack.OutParms, RetValueAddress); // call Lua function...
    }

    if (!bSuccess && bUnpackParams)
    {
        FMemMark Mark(FMemStack::Get());
        void* Params = New<uint8>(FMemStack::Get(), ParmsSize, 16);
        SkipCodes(Stack, Params);
    }
}

bool ULuaFunction::CallLuaInternal(lua_State* L, void* InParams, FOutParmRec* OutParams, void* RetValueAddress) const
{
    // prepare parameters for Lua function
    FOutParmRec* OutParam = OutParams;
    for (const FPropertyDesc* Property : Properties)
    {
        if (Property->IsReturnParameter())
            continue;

        if (Property->IsConstOutParameter())
        {
            OutParam = FindOutParamRec(OutParam, Property->GetProperty());
            if (OutParam)
            {
                Property->GetValueInternal(L, OutParam->PropAddr, false);
                OutParam = OutParam->NextOutParm;
                continue;
            }
        }

        Property->GetValue(L, InParams, !(Property->GetProperty()->PropertyFlags & CPF_OutParm));
    }

    // object is also pushed, return is push when return
    int32 NumParams = Properties.Num();
    int32 NumResult = OutPropertyIndices.Num();
    if (ReturnPropertyIndex == INDEX_NONE)
        NumParams++;
    else
        NumResult++;

    const bool bSuccess = ::CallFunction(L, NumParams, NumResult); // pcall
    if (!bSuccess)
        return false;

    // out value
    // suppose out param is also pushed on stack? this is assumed done by user... so we can not trust it
    int32 NumResultOnStack = lua_gettop(L);
    if (NumResult <= NumResultOnStack)
    {
        int32 OutPropertyIndex = -NumResult;
        OutParam = OutParams;

        for (int32 i = 0; i < OutPropertyIndices.Num(); ++i)
        {
            FPropertyDesc* OutProperty = Properties[OutPropertyIndices[i]];
            if (OutProperty->IsReferenceParameter())
            {
                continue;
            }
            OutParam = FindOutParamRec(OutParam, OutProperty->GetProperty());
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
                    OutProperty->CopyBack(OutParam->PropAddr, OutProperty->GetProperty()->ContainerPtrToValuePtr<void>(InParams)); // copy back value to out property
                }
                else
                {
                    // copy it from stack
                    OutProperty->SetValueInternal(L, OutParam->PropAddr, OutPropertyIndex, true); // set value for out property
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
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("%s has return value, but no value found on stack!"), *GetName());
        }
        else
        {
            const FPropertyDesc* ReturnProperty = Properties[ReturnPropertyIndex];

            // set value for blueprint side return property
            const FOutParmRec* RetParam = OutParam ? FindOutParamRec(OutParam, ReturnProperty->GetProperty()) : nullptr;
            if (RetParam)
                ReturnProperty->SetValueInternal(L, RetParam->PropAddr, -1, true);

            // set value for return property
            check(RetValueAddress);
            ReturnProperty->SetValueInternal(L, RetValueAddress, -1, true);
        }
    }

    lua_pop(L, NumResult);
    return true;
}

void ULuaFunction::SkipCodes(FFrame& Stack, void* Params) const
{
    for (TFieldIterator<FProperty> It(this); It && (It->PropertyFlags & CPF_Parm) == CPF_Parm; ++It)
        Stack.Step(Stack.Object, It->ContainerPtrToValuePtr<uint8>(Params));
    check(Stack.PeekCode() == EX_EndFunctionParms);
    Stack.SkipCode(1); // skip EX_EndFunctionParms
}
