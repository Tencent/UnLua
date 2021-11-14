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

#include "ClassDesc.h"
#include "FieldDesc.h"
#include "PropertyDesc.h"
#include "FunctionDesc.h"
#include "ReflectionRegistry.h"
#include "LuaCore.h"
#include "LuaContext.h"
#include "DefaultParamCollection.h"
#include "UnLuaManager.h"
#include "UnLua.h"

/**
 * Class descriptor constructor
 */
FClassDesc::FClassDesc(UStruct *InStruct, const FString &InName, EType InType)
    : Struct(InStruct), ClassName(InName), Type(InType), UserdataPadding(0), Size(0), RefCount(0), Locked(false),FunctionCollection(nullptr)
{   
	GReflectionRegistry.AddToDescSet(this, DESC_CLASS);

    if (InType == EType::CLASS)
    {
        Size = Struct->GetStructureSize();

        // register implemented interfaces
        for (FImplementedInterface &Interface : Class->Interfaces)
        {
            FClassDesc *InterfaceClass = GReflectionRegistry.RegisterClass(Interface.Class);
            RegisterClass(*GLuaCxt, Interface.Class);
        }

        FunctionCollection = GDefaultParamCollection.Find(*ClassName);
    }
    else if (InType == EType::SCRIPTSTRUCT)
    {
        UScriptStruct::ICppStructOps *CppStructOps = ScriptStruct->GetCppStructOps();
        int32 Alignment = CppStructOps ? CppStructOps->GetAlignment() : ScriptStruct->GetMinAlignment();
        Size = CppStructOps ? CppStructOps->GetSize() : ScriptStruct->GetStructureSize();
        UserdataPadding = CalcUserdataPadding(Alignment);       // calculate padding size for userdata
    }
}

/**
 * Class descriptor destructor
 */
FClassDesc::~FClassDesc()
{   
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("~FClassDesc : %s,%p,%d"), *GetName(), this, RefCount);
#endif
    
    UnLua::FAutoStack  AutoStack;              // make sure lua stack is cleaned

	GReflectionRegistry.RemoveFromDescSet(this);

    // remove refs to class,etc ufunction/delegate
    // if (GLuaCxt->IsUObjectValid(Class))
    {
        UUnLuaManager* UnLuaManager = GLuaCxt->GetManager();
        if (UnLuaManager)
        {
            UnLuaManager->CleanUpByClass(Class);
        }
    }

    // remove lua side class tables
    FTCHARToUTF8 Utf8ClassName(*ClassName);
    ClearLibrary(*GLuaCxt, Utf8ClassName.Get());            // clean up related Lua meta table
    ClearLoadedModule(*GLuaCxt, Utf8ClassName.Get());       // clean up required Lua module

    // remove descs within classdesc,etc property/function
    for (TMap<FName, FFieldDesc*>::TIterator It(Fields); It; ++It)
    {
        delete It.Value();
    }
    for (FPropertyDesc *Property : Properties)
    {   
        if (GReflectionRegistry.IsDescValid(Property,DESC_PROPERTY))
        {
            delete Property;
        }
    }
    for (FFunctionDesc *Function : Functions)
    {
        if (GReflectionRegistry.IsDescValid(Function,DESC_FUNCTION))
        {   
            delete Function;
        }
    }

    Fields.Empty();
    Properties.Empty();
    Functions.Empty();
}

void FClassDesc::AddRef()
{   
    TArray<FClassDesc*> DescChain;
    GetInheritanceChain(DescChain);

    DescChain.Insert(this, 0);   // add self
    for (int i = 0; i < DescChain.Num(); ++i)
    {
        DescChain[i]->RefCount++;
    }
}

void FClassDesc::SubRef()
{ 
    TArray<FClassDesc*> DescChain;
    GetInheritanceChain(DescChain);

    DescChain.Insert(this, 0);   // add self
    for (int i = 0; i < DescChain.Num(); ++i)
    {
        DescChain[i]->RefCount--;
    }
}


void FClassDesc::AddLock()
{
    TArray<FClassDesc*> DescChain;
    GetInheritanceChain(DescChain);

    DescChain.Insert(this, 0);   // add self
    for (int i = 0; i < DescChain.Num(); ++i)
    {
        DescChain[i]->Locked = true;
    }
}

void FClassDesc::ReleaseLock()
{
    TArray<FClassDesc*> DescChain;
    GetInheritanceChain(DescChain);

    DescChain.Insert(this, 0);   // add self
    for (int i = 0; i < DescChain.Num(); ++i)
    {
        DescChain[i]->Locked = false;
    }
}

bool FClassDesc::IsLocked()
{
    return Locked;
}

FFieldDesc* FClassDesc::FindField(const char* FieldName)
{
    FFieldDesc** FieldDescPtr = Fields.Find(FieldName);
    return FieldDescPtr ? *FieldDescPtr : nullptr;
}

/**
 * Register a field of this class
 */
FFieldDesc* FClassDesc::RegisterField(FName FieldName, FClassDesc *QueryClass)
{
    if (!Struct)
    {
        return nullptr;
    }

    FFieldDesc *FieldDesc = nullptr;
    FFieldDesc **FieldDescPtr = Fields.Find(FieldName);
    if (FieldDescPtr)
    {
        FieldDesc = *FieldDescPtr;
    }
    else
    {
        // a property or a function ?
        FProperty *Property = Struct->FindPropertyByName(FieldName);
        UFunction *Function = (!Property && Type == EType::CLASS) ? Class->FindFunctionByName(FieldName) : nullptr;
        bool bValid = Property || Function;
        if (!bValid && Type == EType::SCRIPTSTRUCT && !Struct->IsNative())
        {
            FString FieldNameStr = FieldName.ToString();
            const int32 GuidStrLen = 32;
            const int32 MinimalPostfixlen = GuidStrLen + 3;
            for (TFieldIterator<FProperty> PropertyIt(Struct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); PropertyIt; ++PropertyIt)
            {
                FString DisplayName = (*PropertyIt)->GetName();
                if (DisplayName.Len() > MinimalPostfixlen)
                {
                    DisplayName = DisplayName.LeftChop(GuidStrLen + 1);
                    int32 FirstCharToRemove = INDEX_NONE;
                    if (DisplayName.FindLastChar(TCHAR('_'), FirstCharToRemove))
                    {
                        DisplayName = DisplayName.Mid(0, FirstCharToRemove);
                    }
                }

                if (DisplayName == FieldNameStr)
                {
                    Property = *PropertyIt;
                    break;
                }
            }

            bValid = Property != nullptr;
        }
        if (!bValid)
        {
            return nullptr;
        }

        UStruct *OuterStruct = Property ? Cast<UStruct>(GetPropertyOuter(Property)) : Cast<UStruct>(Function->GetOuter());
        if (OuterStruct)
        {
            if (OuterStruct != Struct)
            {   
                FClassDesc *OuterClass = (FClassDesc*)GReflectionRegistry.RegisterClass(OuterStruct);
                check(OuterClass);
                return OuterClass->RegisterField(FieldName, QueryClass);
            }

            // create new Field descriptor
            FieldDesc = new FFieldDesc;
            FieldDesc->QueryClass = QueryClass;
            FieldDesc->OuterClass = this;
            Fields.Add(FieldName, FieldDesc);
            if (Property)
            {
                FieldDesc->FieldIndex = Properties.Add(FPropertyDesc::Create(Property));        // index of property descriptor
                ++FieldDesc->FieldIndex;
            }
            else
            {
                check(Function);
                FParameterCollection *DefaultParams = FunctionCollection ? FunctionCollection->Functions.Find(FieldName) : nullptr;
                FieldDesc->FieldIndex = Functions.Add(new FFunctionDesc(Function, DefaultParams, INDEX_NONE));  // index of function descriptor
                ++FieldDesc->FieldIndex;
                FieldDesc->FieldIndex = -FieldDesc->FieldIndex;
            }
        }
    }
    return FieldDesc;
}

/**
 * Get class inheritance chain
 */
void FClassDesc::GetInheritanceChain(TArray<FString> &InNameChain, TArray<UStruct*> &InStructChain)
{
    check(Type != EType::UNKNOWN);

    InNameChain.Empty();
    InStructChain.Empty();

    if (GLuaCxt->IsUObjectValid(Struct))
    {   
        if (NameChain.Num() <= 0)
        {
            UStruct* SuperStruct = Struct->GetInheritanceSuper();
            while (SuperStruct)
            {
                FString Name = FString::Printf(TEXT("%s%s"), SuperStruct->GetPrefixCPP(), *SuperStruct->GetName());
                NameChain.Add(Name);
                StructChain.Add(SuperStruct);
                SuperStruct = SuperStruct->GetInheritanceSuper();
            }
        }

        InNameChain = NameChain;
        InStructChain = StructChain;
    }
}

void FClassDesc::GetInheritanceChain(TArray<FClassDesc*>& DescChain)
{
    TArray<FString> InNameChain;
    TArray<UStruct*> InStructChain;
    GetInheritanceChain(InNameChain, InStructChain);

    for (int i = 0; i < NameChain.Num(); ++i)
    {
        FClassDesc* ClassDesc = GReflectionRegistry.FindClass(TCHAR_TO_UTF8(*NameChain[i]));
        if (ClassDesc)
        {
            DescChain.Add(ClassDesc);
        }
        else
        {
            UE_LOG(LogUnLua,Warning,TEXT("GetInheritanceChain : ClassDesc %s in inheritance chain %s not found"), *NameChain[i],*GetName());
        }
    }
}

FClassDesc::EType FClassDesc::GetType(UStruct* InStruct)
{
    EType Type = EType::UNKNOWN;
    UScriptStruct* ScriptStruct = Cast<UScriptStruct>(InStruct);
    if (ScriptStruct)
    {
        Type = EType::SCRIPTSTRUCT;
    }
    else
    {
        UClass* Class = Cast<UClass>(InStruct);
        if (Class)
        {
            Type = EType::CLASS;
        }
    }
    return Type;
}
