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

#include "ReflectionRegistry.h"
#include "LuaCore.h"

/**
 * Clean up
 */
void FReflectionRegistry::Cleanup()
{
    for (TMap<FName, FClassDesc*>::TIterator It(Name2Classes); It; ++It)
    {
        delete It.Value();
    }
    for (TMap<FName, FEnumDesc*>::TIterator It(Enums); It; ++It)
    {
        delete It.Value();
    }
    for (TMap<UFunction*, FFunctionDesc*>::TIterator It(Functions); It; ++It)
    {
        delete It.Value();
    }
    Name2Classes.Empty();
    Struct2Classes.Empty();
    NonNativeStruct2Classes.Empty();
    Enums.Empty();
    Functions.Empty();
}

/**
 * Unregister a class
 */
bool FReflectionRegistry::UnRegisterClass(FClassDesc *ClassDesc)
{
    if (ClassDesc)
    {
        FName Name(ClassDesc->GetFName());
        UStruct *Struct = ClassDesc->AsStruct();
        if (!Struct)
        {
            delete ClassDesc;
            return true;
        }
        else if (ClassDesc->Release())
        {
            Name2Classes.Remove(Name);
            Struct2Classes.Remove(Struct);
            NonNativeStruct2Classes.Remove(Struct);
            return true;
        }
    }
    return false;
}

/**
 * Register a class 
 *
 * @param InName - class name
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(const TCHAR *InName, TArray<FClassDesc*> *OutChain)
{
    const TCHAR *Name = (InName[0] == 'U' || InName[0] == 'A' || InName[0] == 'F' || InName[0] == 'E') ? InName + 1 : InName;
    UStruct *Struct = FindObject<UStruct>(ANY_PACKAGE, Name);       // find first
    if (!Struct)
    {
        Struct = LoadObject<UStruct>(nullptr, Name);                // load if not found
    }
    return RegisterClass(Struct, OutChain);
}

/**
 * Register a class
 *
 * @param InStruct - UStruct
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(UStruct *InStruct, TArray<FClassDesc*> *OutChain)
{
    if (!InStruct)
    {
        return nullptr;
    }

    FClassDesc::EType Type = FClassDesc::GetType(InStruct);
    if (Type == FClassDesc::EType::UNKNOWN)
    {
        return nullptr;
    }

    FClassDesc **ClassDescPtr = Struct2Classes.Find(InStruct);
    if (ClassDescPtr)       // already registered ?
    {
        GetClassChain(*ClassDescPtr, OutChain);
        return *ClassDescPtr;
    }

    FString ClassName = FString::Printf(TEXT("%s%s"), InStruct->GetPrefixCPP(), *InStruct->GetName());
    FClassDesc *ClassDesc = RegisterClassInternal(ClassName, InStruct, Type);
    GetClassChain(ClassDesc, OutChain);
    return ClassDesc;
}

bool FReflectionRegistry::NotifyUObjectDeleted(const UObjectBase *InObject)
{
    UStruct *Struct = (UStruct*)InObject;
    FClassDesc *ClassDesc = nullptr;
    bool bSuccess = NonNativeStruct2Classes.RemoveAndCopyValue(Struct, ClassDesc);
    if (bSuccess && ClassDesc)
    {
        UE_LOG(LogUnLua, Warning, TEXT("Class/ScriptStruct %s has been GCed by engine!!!"), *ClassDesc->GetName());
        ClassDesc->Reset();
        Name2Classes.Remove(ClassDesc->GetFName());
        Struct2Classes.Remove(Struct);
        return true;
    }
    return false;
}

/**
 * Register a UEnum
 */
FEnumDesc* FReflectionRegistry::RegisterEnum(const TCHAR *InName)
{
    FEnumDesc **EnumDescPtr = Enums.Find(FName(InName));
    if (EnumDescPtr)
    {
        return *EnumDescPtr;
    }

    UEnum *Enum = FindObject<UEnum>(ANY_PACKAGE, InName);
    if (!Enum)
    {
        Enum = LoadObject<UEnum>(nullptr, InName);
    }
    return RegisterEnum(Enum);
}

FEnumDesc* FReflectionRegistry::RegisterEnum(UEnum *InEnum)
{
    if (!InEnum)
    {
        return nullptr;
    }

    FName Name = InEnum->GetFName();
    FEnumDesc **EnumDescPtr = Enums.Find(Name);
    if (EnumDescPtr)
    {
        return *EnumDescPtr;
    }

    FEnumDesc *EnumDesc = new FEnumDesc(InEnum);
    Enums.Add(Name, EnumDesc);
    return EnumDesc;
}

/**
 * Register a UFunction
 */
FFunctionDesc* FReflectionRegistry::RegisterFunction(UFunction *InFunction, int32 InFunctionRef)
{
    FFunctionDesc *Function = nullptr;
    FFunctionDesc **FunctionPtr = Functions.Find(InFunction);
    if (FunctionPtr)
    {
        Function = *FunctionPtr;
        if (InFunctionRef != INDEX_NONE)
        {
            check(Function->FunctionRef == INDEX_NONE || Function->FunctionRef == InFunctionRef);
            Function->FunctionRef = InFunctionRef;
        }
    }
    else
    {
        Function = new FFunctionDesc(InFunction, nullptr, InFunctionRef);
        Functions.Add(InFunction, Function);
    }
    return Function;
}

bool FReflectionRegistry::UnRegisterFunction(UFunction *InFunction)
{
    FFunctionDesc *FunctionDesc = nullptr;
    if (Functions.RemoveAndCopyValue(InFunction, FunctionDesc))
    {
        delete FunctionDesc;
        return true;
    }
    return false;
}

#if ENABLE_CALL_OVERRIDDEN_FUNCTION
bool FReflectionRegistry::AddOverriddenFunction(UFunction *NewFunc, UFunction *OverriddenFunc)
{
    UFunction **OverriddenFuncPtr = OverriddenFunctions.Find(NewFunc);
    if (!OverriddenFuncPtr)
    {
        OverriddenFunctions.Add(NewFunc, OverriddenFunc);
        return true;
    }
    return false;
}

UFunction* FReflectionRegistry::RemoveOverriddenFunction(UFunction *NewFunc)
{
    UFunction *OverriddenFunc = nullptr;
    OverriddenFunctions.RemoveAndCopyValue(NewFunc, OverriddenFunc);
    return OverriddenFunc;
}

UFunction* FReflectionRegistry::FindOverriddenFunction(UFunction *NewFunc)
{
    UFunction **OverriddenFuncPtr = OverriddenFunctions.Find(NewFunc);
    return OverriddenFuncPtr ? *OverriddenFuncPtr : nullptr;
}
#endif

FClassDesc* FReflectionRegistry::RegisterClassInternal(const FString &ClassName, UStruct *Struct, FClassDesc::EType Type)
{
    check(Struct && Type != FClassDesc::EType::UNKNOWN);
    FClassDesc *ClassDesc = new FClassDesc(Struct, ClassName, Type);
    Name2Classes.Add(FName(*ClassName), ClassDesc);
    Struct2Classes.Add(Struct, ClassDesc);
    if (!ClassDesc->IsNative())
    {
        NonNativeStruct2Classes.Add(Struct, ClassDesc);
    }

    FClassDesc *CurrentClass = ClassDesc;
    TArray<FString> NameChain;
    TArray<UStruct*> StructChain;
    ClassDesc->GetInheritanceChain(NameChain, StructChain);
    for (int32 i = 0; i < NameChain.Num(); ++i)
    {
        FClassDesc **Class = Struct2Classes.Find(StructChain[i]);
        if (!Class)
        {
            CurrentClass->Parent = new FClassDesc(StructChain[i], NameChain[i], Type);
            Name2Classes.Add(*NameChain[i], CurrentClass->Parent);
            Struct2Classes.Add(StructChain[i], CurrentClass->Parent);
            if (!CurrentClass->Parent->IsNative())
            {
                NonNativeStruct2Classes.Add(StructChain[i], CurrentClass->Parent);
            }
        }
        else
        {
            CurrentClass->Parent = *Class;
            break;
        }
        CurrentClass = CurrentClass->Parent;
    }

    return ClassDesc;
}

/**
 * get class inheritance chain
 */
void FReflectionRegistry::GetClassChain(FClassDesc *ClassDesc, TArray<FClassDesc*> *OutChain)
{
    if (OutChain)
    {
        while (ClassDesc)
        {
            OutChain->Add(ClassDesc);
            ClassDesc = ClassDesc->Parent;
        }
    }
}

FReflectionRegistry GReflectionRegistry;        // global reflection registry
