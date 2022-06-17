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
#include "LuaCore.h"
#include "DefaultParamCollection.h"
#include "LowLevel.h"
#include "UnLuaModule.h"

/**
 * Class descriptor constructor
 */
FClassDesc::FClassDesc(UStruct* InStruct, const FString& InName)
    : Struct(InStruct), ClassName(InName), UserdataPadding(0), Size(0), FunctionCollection(nullptr)
{
    RawStructPtr = InStruct;
    bIsScriptStruct = InStruct->IsA(UScriptStruct::StaticClass());
    bIsClass = InStruct->IsA(UClass::StaticClass());
    bIsInterface = bIsClass && static_cast<UClass*>(InStruct)->HasAnyClassFlags(CLASS_Interface) && InStruct != UInterface::StaticClass();
    bIsNative = InStruct->IsNative();

    if (bIsClass)
    {
        UClass* Class = AsClass();
        Size = Struct->GetStructureSize();

        // register implemented interfaces
        for (FImplementedInterface& Interface : Class->Interfaces)
        {
            UnLua::FClassRegistry::RegisterReflectedType(Interface.Class);
        }

        FunctionCollection = GDefaultParamCollection.Find(*ClassName);
    }
    else if (bIsScriptStruct)
    {
        UScriptStruct* ScriptStruct = AsScriptStruct();
        UScriptStruct::ICppStructOps* CppStructOps = ScriptStruct->GetCppStructOps();
        int32 Alignment = CppStructOps ? CppStructOps->GetAlignment() : ScriptStruct->GetMinAlignment();
        Size = CppStructOps ? CppStructOps->GetSize() : ScriptStruct->GetStructureSize();
        UserdataPadding = CalcUserdataPadding(Alignment); // calculate padding size for userdata
    }

    UStruct* SuperStruct = Struct->GetInheritanceSuper();
    while (SuperStruct)
    {
        FString SuperName = UnLua::LowLevel::GetMetatableName(InStruct);
        FClassDesc* ClassDesc = UnLua::FClassRegistry::RegisterReflectedType(SuperStruct);
        SuperClasses.Add(ClassDesc);
        SuperStruct = SuperStruct->GetInheritanceSuper();
    }
}

TSharedPtr<FFieldDesc> FClassDesc::FindField(const char* FieldName)
{
    Load();

    TSharedPtr<FFieldDesc>* FieldDescPtr = Fields.Find(FieldName);
    return FieldDescPtr ? *FieldDescPtr : nullptr;
}

/**
 * Register a field of this class
 */
TSharedPtr<FFieldDesc> FClassDesc::RegisterField(FName FieldName, FClassDesc* QueryClass)
{
    Load();

    TSharedPtr<FFieldDesc> FieldDesc = nullptr;
    TSharedPtr<FFieldDesc>* FieldDescPtr = Fields.Find(FieldName);
    if (FieldDescPtr)
    {
        FieldDesc = *FieldDescPtr;
    }
    else
    {
        // a property or a function ?
        FProperty* Property = Struct->FindPropertyByName(FieldName);
        UFunction* Function = (!Property && bIsClass) ? AsClass()->FindFunctionByName(FieldName) : nullptr;
        bool bValid = Property || Function;
        if (!bValid && bIsScriptStruct && !Struct->IsNative())
        {
            FString FieldNameStr = FieldName.ToString();
            const int32 GuidStrLen = 32;
            const int32 MinimalPostfixlen = GuidStrLen + 3;
            for (TFieldIterator<FProperty> PropertyIt(Struct.Get(), EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); PropertyIt; ++PropertyIt)
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

        UStruct* OuterStruct = Property ? Cast<UStruct>(GetPropertyOuter(Property)) : Cast<UStruct>(Function->GetOuter());
        if (OuterStruct)
        {
            if (OuterStruct != Struct)
            {
                FClassDesc* OuterClass = UnLua::FClassRegistry::RegisterReflectedType(OuterStruct);
                check(OuterClass);
                return OuterClass->RegisterField(FieldName, QueryClass);
            }

            // create new Field descriptor
            FieldDesc = MakeShared<FFieldDesc>();
            FieldDesc->QueryClass = QueryClass;
            FieldDesc->OuterClass = this;
            Fields.Add(FieldName, FieldDesc);
            if (Property)
            {
                TSharedPtr<FPropertyDesc> Ptr(FPropertyDesc::Create(Property));
                FieldDesc->FieldIndex = Properties.Add(Ptr); // index of property descriptor
                ++FieldDesc->FieldIndex;
            }
            else
            {
                check(Function);
                FParameterCollection* DefaultParams = FunctionCollection ? FunctionCollection->Functions.Find(FieldName) : nullptr;
                FieldDesc->FieldIndex = Functions.Add(MakeShared<FFunctionDesc>(Function, DefaultParams)); // index of function descriptor
                ++FieldDesc->FieldIndex;
                FieldDesc->FieldIndex = -FieldDesc->FieldIndex;
            }
        }
    }
    return FieldDesc;
}

void FClassDesc::GetInheritanceChain(TArray<FClassDesc*>& DescChain)
{
    DescChain.Add(this);
    DescChain.Append(SuperClasses);
}

void FClassDesc::Load()
{
    if (Struct.IsValid())
    {
        return;
    }

    if (GIsGarbageCollecting)
    {
        return;
    }

    UnLoad();

    FString Name = (ClassName[0] == 'U' || ClassName[0] == 'A' || ClassName[0] == 'F') ? ClassName.RightChop(1) : ClassName;
    UStruct* Found = FindObject<UStruct>(ANY_PACKAGE, *Name);
    if (!Found)
        Found = LoadObject<UStruct>(nullptr, *Name);

    Struct = Found;
    RawStructPtr = Found;
}

void FClassDesc::UnLoad()
{
    Fields.Empty();
    Properties.Empty();
    Functions.Empty();

    Struct.Reset();
    RawStructPtr = nullptr;
}
