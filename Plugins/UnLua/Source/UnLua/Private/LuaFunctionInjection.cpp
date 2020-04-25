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

#include "LuaFunctionInjection.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "Misc/MemStack.h"
#include "GameFramework/Actor.h"

#define CLEAR_INTERNAL_NATIVE_FLAG_DURING_DUPLICATION 1

/**
 * Custom thunk function to call Lua function
 */
DEFINE_FUNCTION(FLuaInvoker::execCallLua)
{
    bool bUnpackParams = false;
    UFunction *Func = Stack.Node;
    FFunctionDesc *FuncDesc = nullptr;
    if (Stack.CurrentNativeFunction)
    {
        if (Func != Stack.CurrentNativeFunction)
        {
            Func = Stack.CurrentNativeFunction;
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
            FMemory::Memcpy(&FuncDesc, &Stack.CurrentNativeFunction->Script[1], sizeof(FuncDesc));
#endif
            bUnpackParams = true;
        }
        else
        {
            if (Func->GetNativeFunc() == (FNativeFuncPtr)&FLuaInvoker::execCallLua)
            {
                check(*Stack.Code == EX_CallLua);
                Stack.SkipCode(1);      // skip EX_CallLua only when called from native func
            }
        }
    }

#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    if (!FuncDesc)
    {
        FMemory::Memcpy(&FuncDesc, Stack.Code, sizeof(FuncDesc));
        Stack.SkipCode(sizeof(FuncDesc));       // skip 'FFunctionDesc' pointer
    }
#else
    FuncDesc = GReflectionRegistry.RegisterFunction(Func);
#endif

    bool bRpcCall = false;
#if SUPPORTS_RPC_CALL
    AActor *Actor = Cast<AActor>(Stack.Object);
    if (!Actor)
    {
        UActorComponent *ActorComponent = Cast<UActorComponent>(Stack.Object);
        if (ActorComponent)
        {
            Actor = ActorComponent->GetOwner();
        }
    }
    if (Actor)
    {
        ENetMode NetMode = Actor->GetNetMode();
        if ((Func->HasAnyFunctionFlags(FUNC_NetClient) && NetMode == NM_Client) || (Func->HasAnyFunctionFlags(FUNC_NetServer) && (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer)))
        {
            bRpcCall = true;
        }
    }
#endif

    bool bSuccess = FuncDesc->CallLua(Context, Stack, (void*)RESULT_PARAM, bRpcCall, bUnpackParams);
    if (!bSuccess && bUnpackParams)
    {
        FMemMark Mark(FMemStack::Get());
        void *Params = New<uint8>(FMemStack::Get(), Func->ParmsSize, 16);
        for (TFieldIterator<FProperty> It(Func); It && (It->PropertyFlags & CPF_Parm) == CPF_Parm; ++It)
        {
            Stack.Step(Stack.Object, It->ContainerPtrToValuePtr<uint8>(Params));
        }
        Stack.SkipCode(1);          // skip EX_EndFunctionParms
    }
}

/**
 * Register thunk function for new opcode
 */
extern uint8 GRegisterNative(int32 NativeBytecodeIndex, const FNativeFuncPtr& Func);
static FNativeFunctionRegistrar CallLuaRegistrar(UObject::StaticClass(), "execCallLua", (FNativeFuncPtr)&FLuaInvoker::execCallLua);
static uint8 CallLuaBytecode = GRegisterNative(EX_CallLua, (FNativeFuncPtr)&FLuaInvoker::execCallLua);


/**
 * Get all UFUNCTIONs that can be overrode
 */
void GetOverridableFunctions(UClass *Class, TMap<FName, UFunction*> &Functions)
{
    if (!Class)
    {
        return;
    }

    // all 'BlueprintEvent'
    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::IncludeInterfaces); It; ++It)
    {
        UFunction *Function = *It;
        if (Function->HasAnyFunctionFlags(FUNC_BlueprintEvent))
        {
            FName FuncName = Function->GetFName();
            UFunction **FuncPtr = Functions.Find(FuncName);
            if (!FuncPtr)
            {
                Functions.Add(FuncName, Function);
            }
        }
    }

    // all 'RepNotifyFunc'
    for (int32 i = 0; i < Class->ClassReps.Num(); ++i)
    {
        FProperty *Property = Class->ClassReps[i].Property;
        if (Property->HasAnyPropertyFlags(CPF_RepNotify))
        {
            UFunction *Function = Class->FindFunctionByName(Property->RepNotifyFunc);
            if (Function)
            {
                UFunction **FuncPtr = Functions.Find(Property->RepNotifyFunc);
                if (!FuncPtr)
                {
                    Functions.Add(Property->RepNotifyFunc, Function);
                }
            }
        }
    }
}

/**
 * Only used to get offset of 'Offset_Internal'
 */
#if ENGINE_MINOR_VERSION < 25
struct FFakeProperty : public UField
#else
struct FFakeProperty : public FField
#endif
{
    int32       ArrayDim;
    int32       ElementSize;
    uint64      PropertyFlags;
    uint16      RepIndex;
    TEnumAsByte<ELifetimeCondition> BlueprintReplicationCondition;
    int32       Offset_Internal;
};

/**
 * 1. Duplicate template UFUNCTION
 * 2. Add duplicated UFUNCTION to class' function map
 * 3. Register 'FFunctionDesc'
 * 4. Add to root if necessary
 */
UFunction* DuplicateUFunction(UFunction *TemplateFunction, UClass *OuterClass, FName NewFuncName)
{
    static int32 Offset = offsetof(FFakeProperty, Offset_Internal);
    static FArchive Ar;         // dummy archive used for FProperty::Link()

#if CLEAR_INTERNAL_NATIVE_FLAG_DURING_DUPLICATION
    FObjectDuplicationParameters DuplicationParams(TemplateFunction, OuterClass);
    DuplicationParams.DestName = NewFuncName;
    DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
    UFunction *NewFunc = Cast<UFunction>(StaticDuplicateObjectEx(DuplicationParams));
#else
    UFunction *NewFunc = DuplicateObject(TemplateFunction, OuterClass, NewFuncName);
#endif
    NewFunc->PropertiesSize = TemplateFunction->PropertiesSize;
    NewFunc->MinAlignment = TemplateFunction->MinAlignment;
    int32 NumParams = NewFunc->NumParms;
    if (NumParams > 0)
    {
        NewFunc->PropertyLink = CastField<FProperty>(GetChildProperties(NewFunc));
        FProperty *SrcProperty = CastField<FProperty>(GetChildProperties(TemplateFunction));
        FProperty *DestProperty = NewFunc->PropertyLink;
        while (true)
        {
            check(SrcProperty && DestProperty);
            DestProperty->Link(Ar);
            //DestProperty->ArrayDim = SrcProperty->ArrayDim;
            //DestProperty->ElementSize = SrcProperty->ElementSize;
            //DestProperty->PropertyFlags = SrcProperty->PropertyFlags;
            DestProperty->RepIndex = SrcProperty->RepIndex;
            *((int32*)((uint8*)DestProperty + Offset)) = *((int32*)((uint8*)SrcProperty + Offset)); // set Offset_Internal (Offset_Internal set by DestProperty->Link(Ar) is incorrect because of incorrect Outer class)
            if (--NumParams < 1)
            {
                break;
            }
            DestProperty->PropertyLinkNext = CastField<FProperty>(DestProperty->Next);
            DestProperty = DestProperty->PropertyLinkNext;
            SrcProperty = SrcProperty->PropertyLinkNext;
        }
    }
    OuterClass->AddFunctionToFunctionMap(NewFunc, NewFuncName);
    //GReflectionRegistry.RegisterFunction(NewFunc);
    NewFunc->ClearInternalFlags(EInternalObjectFlags::Native);
    if (GUObjectArray.DisregardForGCEnabled() || GUObjectClusters.GetNumAllocatedClusters())
    {
        NewFunc->AddToRoot();
    }
    return NewFunc;
}

/**
 * 1. Remove duplicated UFUNCTION from class' function map
 * 2. Unregister 'FFunctionDesc'
 * 3. Remove from root if necessary
 * 4. Clear 'Native' flag if necessary
 */
void RemoveUFunction(UFunction *Function, UClass *OuterClass)
{
    OuterClass->RemoveFunctionFromFunctionMap(Function);
    GReflectionRegistry.UnRegisterFunction(Function);
    if (GUObjectArray.DisregardForGCEnabled() || GUObjectClusters.GetNumAllocatedClusters())
    {
        Function->RemoveFromRoot();
    }
#if !CLEAR_INTERNAL_NATIVE_FLAG_DURING_DUPLICATION
    if (Function->IsNative())
    {
        Function->ClearInternalFlags(EInternalObjectFlags::Native);
        for (TFieldIterator<FProperty> It(Function); It; ++It)
        {
            FProperty *Property = *It;
            Property->ClearInternalFlags(EInternalObjectFlags::Native);
        }
    }
#endif
}

/**
 * 1. Replace thunk function
 * 2. Insert special opcodes if necessary
 */
void OverrideUFunction(UFunction *Function, FNativeFuncPtr NativeFunc, void *Userdata, bool bInsertOpcodes)
{
    if (!Function->HasAnyFunctionFlags(FUNC_Net) || Function->HasAnyFunctionFlags(FUNC_Native))
    {
        Function->SetNativeFunc(NativeFunc);
    }

    if (Function->Script.Num() < 1)
    {
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
        if (bInsertOpcodes)
        {
            Function->Script.Add(EX_CallLua);
            int32 Index = Function->Script.AddZeroed(sizeof(Userdata));
            FMemory::Memcpy(Function->Script.GetData() + Index, &Userdata, sizeof(Userdata));
            Function->Script.Add(EX_Return);
            Function->Script.Add(EX_Nothing);
        }
        else
        {
            int32 Index = Function->Script.AddZeroed(sizeof(Userdata));
            FMemory::Memcpy(Function->Script.GetData() + Index, &Userdata, sizeof(Userdata));
        }
#else
        Function->Script.Add(EX_CallLua);
        Function->Script.Add(EX_Return);
        Function->Script.Add(EX_Nothing);
#endif
    }
}
