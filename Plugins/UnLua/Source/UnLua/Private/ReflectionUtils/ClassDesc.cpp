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

/**
 * Class descriptor constructor
 */
FClassDesc::FClassDesc(UStruct *InStruct, const FString &InName, EType InType)
    : Struct(InStruct), ClassName(InName), ClassFName(*InName), ClassAnsiName(*InName), Type(InType), UserdataPadding(0), Size(0), RefCount(0), Parent(nullptr), FunctionCollection(nullptr)
{
    if (InType == EType::CLASS)
    {
        Size = Struct->GetStructureSize();

        // register implemented interfaces
        for (FImplementedInterface &Interface : Class->Interfaces)
        {
            FClassDesc *InterfaceClass = GReflectionRegistry.RegisterClass(Interface.Class);
            RegisterClass(*GLuaCxt, Interface.Class);
        }

        FunctionCollection = GDefaultParamCollection.Find(ClassFName);
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
    for (TMap<FName, FFieldDesc*>::TIterator It(Fields); It; ++It)
    {
        delete It.Value();
    }
    for (FPropertyDesc *Property : Properties)
    {
        delete Property;
    }
    for (FFunctionDesc *Function : Functions)
    {
        delete Function;
    }
}

/**
 * Release a class descriptor
 */
bool FClassDesc::Release(bool bKeepAlive)
{
    --RefCount;     // dec reference count
    if (!RefCount && !Struct->IsNative())
    {
        if (!bKeepAlive)
        {
            delete this;
            return true;
        }
    }
    return false;
}

/**
 * Reset a class descriptor
 */
void FClassDesc::Reset()
{
    Struct = nullptr;
    Type = EType::UNKNOWN;

    ClearLibrary(*GLuaCxt, ClassAnsiName.Get());            // clean up related Lua meta table
    ClearLoadedModule(*GLuaCxt, ClassAnsiName.Get());       // clean up required Lua module
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
void FClassDesc::GetInheritanceChain(TArray<FString> &NameChain, TArray<UStruct*> &StructChain) const
{
    check(Type != EType::UNKNOWN);
    UStruct *SuperStruct = Struct->GetInheritanceSuper();
    while (SuperStruct)
    {
        FString Name = FString::Printf(TEXT("%s%s"), SuperStruct->GetPrefixCPP(), *SuperStruct->GetName());
        NameChain.Add(Name);
        StructChain.Add(SuperStruct);
        SuperStruct = SuperStruct->GetInheritanceSuper();
    }
}
