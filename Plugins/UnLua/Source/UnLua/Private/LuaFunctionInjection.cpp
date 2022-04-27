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

#include "LuaFunction.h"
#include "Misc/MemStack.h"
#include "GameFramework/Actor.h"

/**
 * Custom thunk function to call Lua function
 */
DEFINE_FUNCTION(FLuaInvoker::execCallLua)
{
    const auto LuaFunction = Cast<ULuaFunction>(Stack.CurrentNativeFunction);
    LuaFunction->Call(Context, Stack, RESULT_PARAM);
}

/**
 * Register thunk function for new opcode
 */
extern uint8 GRegisterNative(int32 NativeBytecodeIndex, const FNativeFuncPtr& Func);
static FNativeFunctionRegistrar CallLuaRegistrar(UObject::StaticClass(), "execCallLua", (FNativeFuncPtr)&FLuaInvoker::execCallLua);
static uint8 CallLuaBytecode = GRegisterNative(EX_CallLua, (FNativeFuncPtr)&FLuaInvoker::execCallLua);


/**
 * Whether the UFunction is overridable
 */
bool IsOverridable(UFunction *Function)
{
    check(Function);

    static const uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
    static const uint32 FlagResult = FUNC_Native | FUNC_Event;
    return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

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
        if (IsOverridable(Function))
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
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 25
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
 * 1. Remove duplicated UFUNCTION from class' function map
 * 2. Unregister 'FFunctionDesc'
 * 3. Remove from root if necessary
 * 4. Clear 'Native' flag if necessary
 */
void RemoveUFunction(UFunction* Function, UClass* OuterClass)
{
    UE_LOG(UnLuaDelegate, Verbose, TEXT("Clean %s"), *Function->GetName());

    if (!OuterClass->HasAnyFlags(RF_BeginDestroyed) && !Function->HasAnyFlags(RF_BeginDestroyed))
    {
#if UNLUA_ENABLE_DEBUG != 0
        const FString Result = OuterClass->FindFunctionByName(*Function->GetName()) ? "OK" : "Not Exists";
        UE_LOG(LogUnLua, Log, TEXT("RemoveUFunction: [%p], [%s] From Class : [%p], [%s] Result=%s"), Function, *Function->GetName(), OuterClass, *OuterClass->GetFullName(), *Result);
#endif
        OuterClass->RemoveFunctionFromFunctionMap(Function);

        if(OuterClass->Children == Function)
        {
            OuterClass->Children = Function->Next;
        }
        else
        {
            UField* Previous = OuterClass->Children;
            while(Previous && Previous->Next != Function)
                Previous = Previous->Next;
            if(Previous)
                Previous->Next = Function->Next;
        }
    }
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
