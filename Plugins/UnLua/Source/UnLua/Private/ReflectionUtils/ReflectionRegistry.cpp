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

#include "DelegateHelper.h"
#include "LuaCore.h"
#include "UnLuaManager.h"
#include "PropertyDesc.h"
#include "UEObjectReferencer.h"
#include "UnLua.h"

FReflectionRegistry::FReflectionRegistry()
{
    PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddLambda([this]()
    {
        this->PostGarbageCollect();
    });
}

FReflectionRegistry::~FReflectionRegistry()
{
    Cleanup();
    FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);
}

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
    Classes.Empty();
    Name2Classes.Empty();
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
    return ClassDesc ? *ClassDesc : nullptr;
}

/**
 * Unregister a class
 */
void FReflectionRegistry::TryUnRegisterClass(FClassDesc* ClassDesc, bool bForce)
{
    ClassDesc->SubRef();

#if UNLUA_ENABLE_DEBUG != 0
    if (bForce && ClassDesc->GetRefCount() != 0)
        UE_LOG(LogUnLua, Log, TEXT("FReflectionRegistry::TryUnRegisterClass: Force unregister class %s ref=%i"), *ClassDesc->GetName(), ClassDesc->GetRefCount());
#endif

    if (!bForce && ClassDesc->GetRefCount() > 0)
        return;

    if (IsInClassWhiteSet(ClassDesc->GetName()))
        return;

    ClassDesc->UnLoad();
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
        Struct = LoadObject<UStruct>(nullptr, *Name);                // load if not found

    if (!Struct)
        return nullptr;
    
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
	// already registered ?
	FString ClassName = FString::Printf(TEXT("%s%s"), InStruct->GetPrefixCPP(), *InStruct->GetName());
	FClassDesc *ClassDesc = GReflectionRegistry.FindClass(TCHAR_TO_UTF8(*ClassName));
    if (!ClassDesc)
    {
        ClassDesc = RegisterClassInternal(ClassName, InStruct);
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
            if (!UnLua::IsUObjectValid((*EnumDesc)->GetEnum()))
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
    if (!UnLua::IsUObjectValid(InEnum))
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
FFunctionDesc* FReflectionRegistry::RegisterFunction(UFunction* InFunction, int32 InFunctionRef)
{
    TSharedPtr<FFunctionDesc> Desc = Functions.FindRef(InFunction);
    if (Desc)
    {
        if (InFunctionRef != INDEX_NONE)
        {
            checkf(Desc->FunctionRef == INDEX_NONE || Desc->FunctionRef == InFunctionRef, TEXT("Function(%p %s) Owner=%s Desc(%s))"), InFunction, *InFunction->GetName(),
                   *InFunction->GetOwnerClass()->GetName(), *Desc->FuncName)
            Desc->FunctionRef = InFunctionRef;
        }
        return Desc.Get();
    }

    Desc = MakeShareable(new FFunctionDesc(InFunction, nullptr, InFunctionRef));
    Functions.Add(InFunction, Desc);
    return Desc.Get();
}

bool FReflectionRegistry::UnRegisterFunction(UFunction* InFunction)
{
    if (InFunction->HasAnyFlags(RF_BeginDestroyed))
        return false;
    TSharedPtr<FFunctionDesc> FunctionDesc;
    return Functions.RemoveAndCopyValue(InFunction, FunctionDesc);
}

bool FReflectionRegistry::NotifyUObjectDeleted(const UObjectBase* InObject)
{   
    UObject* Object = (UObject*)InObject;

    FClassDesc* ClassDesc;
    if (Classes.RemoveAndCopyValue((UStruct*)Object, ClassDesc))
    {
        ClassDesc->UnLoad();
        return true;
    }

    // non class object,check class
    // check lua use this object or not
    bool bNeedProcess = GCSet.Contains(Object);
    if (bNeedProcess)
    {
        GCSet.Remove(Object);
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
        FString ClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
        ClassDesc = FindClass(TCHAR_TO_UTF8(*ClassName));
        if (ClassDesc)
        {
#if UNLUA_ENABLE_DEBUG != 0
            UE_LOG(LogUnLua, Log, TEXT("FReflectionRegistry::NotifyUObjectDeleted:%p,%s"),Object, *ClassName);
#endif
            TryUnRegisterClass(ClassDesc);
        }
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
    if (NewFunc->HasAnyFlags(RF_BeginDestroyed))
        return nullptr;
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

void FReflectionRegistry::AddToGCSet(UObject* InObject)
{
    FClassDesc* ClassDesc = Classes.FindRef((UClass*)InObject);
    if (ClassDesc)
        TryUnRegisterClass(ClassDesc);
    else
        GObjectReferencer.RemoveObjectRef(InObject);

    FDelegateHelper::Remove(InObject);
    GCSet.Add(InObject, true);
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

FClassDesc* FReflectionRegistry::RegisterClassInternal(const FString &ClassName, UStruct *Struct)
{
    // !!!Fix!!!
    // desc pool is needed
    check(Struct);
    FClassDesc *ClassDesc = new FClassDesc(Struct, ClassName);
    Classes.Add(Struct, ClassDesc);
    Name2Classes.Add(FName(*ClassName), ClassDesc);
    
    TArray<FString> NameChain;
    TArray<UStruct*> StructChain;
    ClassDesc->GetInheritanceChain(NameChain, StructChain);
    for (int32 i = 0; i < NameChain.Num(); ++i)
    {
        FClassDesc* ClassDescParent = FindClass(TCHAR_TO_UTF8(*NameChain[i]));
        if (!ClassDescParent)
        {   
            ClassDescParent = new FClassDesc(StructChain[i], NameChain[i]);
            Classes.Add(StructChain[i], ClassDescParent);
            Name2Classes.Add(*NameChain[i], ClassDescParent);
        }
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("RegisterClass : %s,%p"), *ClassName, ClassDesc);
#endif

    return ClassDesc;
}

void FReflectionRegistry::PostGarbageCollect()
{
    for (auto It = Functions.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }

    for (auto It = OverriddenFunctions.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
UNLUA_API FReflectionRegistry GReflectionRegistry;        // global reflection registry
