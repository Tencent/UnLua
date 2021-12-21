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
#include "UnLuaManager.h"
#include "UEObjectReferencer.h"
#include "PropertyDesc.h"
#include "UnLua.h"

/**
 * Clean up
 */
void FReflectionRegistry::Cleanup()
{
    for (TMap<FName, FClassDesc*>::TIterator It(Name2Classes); It; ++It)
    {
        if (GReflectionRegistry.IsDescValid(It.Value(), DESC_CLASS))
        {
            delete It.Value();
        }
    }
    for (TMap<FName, FEnumDesc*>::TIterator It(Enums); It; ++It)
    {
        delete It.Value();
    }
    for (TMap<UFunction*, FFunctionDesc*>::TIterator It(Functions); It; ++It)
    {  
        if (GReflectionRegistry.IsDescValid(It.Value(), DESC_FUNCTION))
        {
            delete It.Value();
        }
    }
    Name2Classes.Empty();
    Struct2Classes.Empty();
    Enums.Empty();
    Functions.Empty();
	DescSet.Empty();
    GCSet.Empty();
    ClassWhiteSet.Empty();
}

FClassDesc* FReflectionRegistry::FindClass(const char* InName)
{
    FName ClassName = InName;
    FClassDesc** ClassDesc = Name2Classes.Find(ClassName);
    if (ClassDesc)
    {
        // release invalid desc if found
        if (!IsDescValid(*ClassDesc, DESC_CLASS))
        {
            Name2Classes.Remove(ClassName);
            ClassDesc = nullptr;
        }
        else
        {
            if (!GLuaCxt->IsUObjectValid((*ClassDesc)->AsStruct()))
            {
                UnRegisterClass(*ClassDesc);
                ClassDesc = nullptr;
            }
        }
    }
    return ClassDesc ? *ClassDesc : nullptr;
}

/**
 * Try to unregister a class，may do nothing
 */
void FReflectionRegistry::TryUnRegisterClass(FClassDesc* ClassDesc)
{
    if (IsDescValid(ClassDesc, DESC_CLASS))
    {
        if (!ClassDesc->IsValid())
        {
#if UNLUA_ENABLE_DEBUG != 0
            UE_LOG(LogTemp, Log, TEXT("TryUnRegisterClass : remove empty class desc %s"),*ClassDesc->GetName());
#endif
            // remove empty desc
            UnRegisterClass(ClassDesc);
        }
        else
        {
#if UNLUA_ENABLE_DEBUG != 0
            UE_LOG(LogTemp, Log, TEXT("TryUnRegisterClass : %s,%d,%d,%d,%d"),
                *ClassDesc->GetName(), ClassDesc->GetRefCount(), ClassDesc->IsLocked(), IsInClassWhiteSet(ClassDesc->GetName()), ClassDesc->IsNative());
#endif

            if ((!ClassDesc->IsLocked())
                && (ClassDesc->GetRefCount() <= 0))
            {
                bool bNeedClean = false;
                if (IsInClassWhiteSet(ClassDesc->GetName()))
                {
                    bNeedClean = false;
                }
                else
                {
                    if (!ClassDesc->IsNative())
                    {
#if ENABLE_AUTO_CLEAN_NNATCLASS != 0
                        bNeedClean = true;
#endif
                    }
                }

                if (bNeedClean)
                {
                    if (!ClassDesc->IsNative())
                    {
                        GObjectReferencer.RemoveObjectRef(ClassDesc->AsStruct());
                    }

                    UnRegisterClass(ClassDesc);
                }
            }
        }
    }
}

/**
 * Unregister a class
 */
bool FReflectionRegistry::UnRegisterClass(FClassDesc *ClassDesc)
{   
    if (GReflectionRegistry.IsDescValid(ClassDesc, DESC_CLASS))
    {   
        FName Name(ClassDesc->GetName());
        UStruct* Struct = ClassDesc->AsStruct();

        delete ClassDesc;

        // clear classdesc registry 
        Name2Classes.Remove(Name);
        Struct2Classes.Remove(Struct);
    }

    return true;
}

/**
 * Register a class 
 *
 * @param InName - class name
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(const char* InName)
{
    FString Name = (InName[0] == 'U' || InName[0] == 'A' || InName[0] == 'F' || InName[0] == 'E') ? UTF8_TO_TCHAR(InName + 1) : UTF8_TO_TCHAR(InName);
    UStruct *Struct = FindObject<UStruct>(ANY_PACKAGE, *Name);       // find first
    if (!Struct)
    {
        Struct = LoadObject<UStruct>(nullptr, *Name);                // load if not found
    }
    return RegisterClass(Struct);
}

/**
 * Register a class
 *
 * @param InStruct - UStruct
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(UStruct *InStruct)
{
    if (!GLuaCxt->IsUObjectValid(InStruct))
    {
        return nullptr;
    }

    FClassDesc::EType Type = FClassDesc::GetType(InStruct);
    if (Type == FClassDesc::EType::UNKNOWN)
    {
        return nullptr;
    }

	// already registered ?
	FString ClassName = FString::Printf(TEXT("%s%s"), InStruct->GetPrefixCPP(), *InStruct->GetName());
	FClassDesc *ClassDesc = GReflectionRegistry.FindClass(TCHAR_TO_UTF8(*ClassName));
    if (!ClassDesc)
    {
        ClassDesc = RegisterClassInternal(ClassName, InStruct, Type);
    }

    return ClassDesc;
}


FEnumDesc* FReflectionRegistry::FindEnum(const char* InName)
{
    FName EnumName(InName);
    FEnumDesc** EnumDesc = Enums.Find(EnumName);
    if (EnumDesc)
    {
        // release invalid desc if found
        if (!IsDescValid(*EnumDesc, DESC_ENUM))
        {
            Enums.Remove(EnumName);
            EnumDesc = nullptr;
        }
        else
        {
            if (!GLuaCxt->IsUObjectValid((*EnumDesc)->GetEnum()))
            {
                UnRegisterEnum(*EnumDesc);
                EnumDesc = nullptr;
            }

        }
    }
    return EnumDesc ? *EnumDesc : nullptr;
}


/**
 * Register a UEnum
 */
FEnumDesc* FReflectionRegistry::RegisterEnum(const char* InName)
{   
    FString Name = UTF8_TO_TCHAR(InName);
    UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, *Name);
    if (!Enum)
    {
        Enum = LoadObject<UEnum>(nullptr, *Name);
        if (!Enum)
        {
            return nullptr;
        }
    }

    FEnumDesc *EnumDesc = FindEnum(InName);
    if ((EnumDesc)
        && (EnumDesc->GetEnum() != Enum))
    {
        UnRegisterEnum(EnumDesc);
        EnumDesc = nullptr;
    }
        
    if (EnumDesc)
    {
        return EnumDesc;
    }
    else
    {
        return RegisterEnum(Enum);
    }
}

FEnumDesc* FReflectionRegistry::RegisterEnum(UEnum *InEnum)
{
    if (!GLuaCxt->IsUObjectValid(InEnum))
    {
        return nullptr;
    }

    FTCHARToUTF8 Name(*InEnum->GetName());
    FEnumDesc* EnumDesc = FindEnum(Name.Get());
    if ((EnumDesc)
        && (EnumDesc->GetEnum() != InEnum))
    {
        UnRegisterEnum(EnumDesc);
        EnumDesc = nullptr;
    }

    if (EnumDesc)
    {
        return EnumDesc;
    }
    else
    {
        if (InEnum->IsA<UUserDefinedEnum>())
        {
            EnumDesc = new FEnumDesc(InEnum, FEnumDesc::EType::UserDefinedEnum);
        }
        else
        {
            EnumDesc = new FEnumDesc(InEnum);
        }
        Enums.Add(Name.Get(), EnumDesc);
    }

    return EnumDesc;
}

bool FReflectionRegistry::UnRegisterEnum(const FEnumDesc* EnumDesc)
{
    if (IsDescValid((void*)EnumDesc,DESC_ENUM))
    {
        Enums.Remove(*EnumDesc->GetName());
        delete EnumDesc;
    }

    return true;
}

/**
 * Register a UFunction
 */
FFunctionDesc* FReflectionRegistry::RegisterFunction(UFunction *InFunction, int32 InFunctionRef)
{
    FFunctionDesc **FunctionPtr = Functions.Find(InFunction);
    if ((FunctionPtr)
        && (!IsDescValid(*FunctionPtr, DESC_FUNCTION)))
    {   
        Functions.Remove(InFunction);
        FunctionPtr = nullptr;
    }

    if ((FunctionPtr)
        &&((*FunctionPtr)->GetFunction() != InFunction))
    {
        UnRegisterFunction(InFunction);
        FunctionPtr = nullptr;
    }

    FFunctionDesc* Function = nullptr;
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
    FFunctionDesc** FunctionDesc = Functions.Find(InFunction);
    if (FunctionDesc)
    {   
        Functions.Remove(InFunction);

        if (IsDescValid(*FunctionDesc, DESC_FUNCTION))
        {
            delete *FunctionDesc;
            return true;
        }
    }

    return false;
}

bool FReflectionRegistry::NotifyUObjectDeleted(const UObjectBase* InObject)
{   
    UObject* Object = (UObject*)InObject;

    FClassDesc* ClassDesc = nullptr;
    if (Struct2Classes.RemoveAndCopyValue((UStruct*)InObject, ClassDesc))
    {   
#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogUnLua, Log, TEXT("FReflectionRegistry::NotifyUObjectDeleted: Class was gced by UE,so cleanup class %s"),*ClassDesc->GetName());
#endif
        // class,ignore ref count
        UnRegisterClass(ClassDesc);

        return true;
    }
    else
    {
        // non class object,check class
        
        // check lua use this object or not
        bool bNeedProcess = IsInGCSet(Object);
        if (bNeedProcess)
        {
            RemoveFromGCSet(Object);
        }
        else
        {
            lua_State* L = UnLua::GetState();
            if (L)
            {
                lua_getfield(UnLua::GetState(), LUA_REGISTRYINDEX, "ObjectMap");            // get the object instance from 'ObjectMap'
                lua_pushlightuserdata(L, Object);
                int32 Type = lua_rawget(L, -2);
                lua_pop(L, 2);

                bNeedProcess = (Type == LUA_TTABLE || Type == LUA_TUSERDATA);
            }
        }

        if (bNeedProcess)
        {
            UClass* Class = Object->GetClass();
            if (GLuaCxt->IsUObjectValid(Class))
            {
                FString ClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
                ClassDesc = FindClass(TCHAR_TO_UTF8(*ClassName));
                if (ClassDesc)
                {
                    ClassDesc->SubRef();

#if UNLUA_ENABLE_DEBUG != 0
                    UE_LOG(LogUnLua, Log, TEXT("FReflectionRegistry::NotifyUObjectDeleted:%p,%s"),Object, *ClassName);
#endif
                    TryUnRegisterClass(ClassDesc);
                }
            }
        }
        
        return false;
    }
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


void FReflectionRegistry::AddToDescSet(void* Desc, EDescType type)
{
	DescSet.Add(Desc, type);
}

void FReflectionRegistry::RemoveFromDescSet(void* Desc)
{
	DescSet.Remove(Desc);
}

bool FReflectionRegistry::IsDescValid(void* Desc, EDescType type)
{   
    EDescType* TypePtr = DescSet.Find(Desc);
    return TypePtr && (*TypePtr == type);
}

bool FReflectionRegistry::IsDescValidWithObjectCheck(void* Desc, EDescType type)
{
    bool bValid = IsDescValid(Desc, type);
    if (bValid)
    {
        switch (type)
        {
        case DESC_CLASS:
            bValid = bValid && ((FClassDesc*)Desc)->IsValid();
            break;
        case DESC_FUNCTION:
            bValid = bValid && ((FFunctionDesc*)Desc)->IsValid();
            break;
        case DESC_PROPERTY:
            bValid = bValid && ((FPropertyDesc*)Desc)->IsValid();
            break;
        case DESC_ENUM:
            bValid = bValid && ((FEnumDesc*)Desc)->IsValid();
            break;
        default:
            bValid = false;
        }
    }

    return bValid;
}

void FReflectionRegistry::AddToGCSet(const UObject* InObject)
{   
    GCSet.Add(InObject,true);
}


void FReflectionRegistry::RemoveFromGCSet(const UObject* InObject)
{
    GCSet.Remove(InObject);
}


bool FReflectionRegistry::IsInGCSet(const UObject* InObject)
{
    return GCSet.Contains(InObject);
}


void FReflectionRegistry::AddToClassWhiteSet(const FString& ClassName)
{
    ClassWhiteSet.Add(ClassName, true);
}

void FReflectionRegistry::RemoveFromClassWhiteSet(const FString& ClassName)
{
    ClassWhiteSet.Remove(ClassName);
}

bool FReflectionRegistry::IsInClassWhiteSet(const FString& ClassName)
{
    return ClassWhiteSet.Contains(ClassName);
}

FClassDesc* FReflectionRegistry::RegisterClassInternal(const FString &ClassName, UStruct *Struct, FClassDesc::EType Type)
{
    // !!!Fix!!!
    // desc pool is needed
    check(Struct && Type != FClassDesc::EType::UNKNOWN);
    FClassDesc *ClassDesc = new FClassDesc(Struct, ClassName, Type);
    Name2Classes.Add(FName(*ClassName), ClassDesc);
    Struct2Classes.Add(Struct, ClassDesc);
    
    FClassDesc *CurrentClass = ClassDesc;
    TArray<FString> NameChain;
    TArray<UStruct*> StructChain;
    ClassDesc->GetInheritanceChain(NameChain, StructChain);
    for (int32 i = 0; i < NameChain.Num(); ++i)
    {
        FClassDesc* ClassDescParent = FindClass(TCHAR_TO_UTF8(*NameChain[i]));
        if (!ClassDescParent)
        {   
            ClassDescParent = new FClassDesc(StructChain[i], NameChain[i], Type);
            Name2Classes.Add(*NameChain[i], ClassDescParent);
            Struct2Classes.Add(StructChain[i], ClassDescParent);
        }
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("RegisterClass : %s,%p"), *ClassName, ClassDesc);
#endif

    return ClassDesc;
}

UNLUA_API FReflectionRegistry GReflectionRegistry;        // global reflection registry
