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

#pragma once

#include "CoreUObject.h"

class FPropertyDesc;
class FFunctionDesc;
class FFieldDesc;

/**
 * Class descriptor
 */
class FClassDesc
{
    friend class FReflectionRegistry;

public:
    enum class EType
    {
        SCRIPTSTRUCT,
        CLASS,
        UNKNOWN
    };

    FClassDesc(UStruct *InStruct, const FString &InName, EType InType);
    ~FClassDesc();

    FORCEINLINE bool IsValid() const { return Type != EType::UNKNOWN && Struct/* && Struct->IsValidLowLevel()*/; }

    FORCEINLINE bool IsScriptStruct() const { return Type == EType::SCRIPTSTRUCT; }

    FORCEINLINE bool IsClass() const { return Type == EType::CLASS; }

    FORCEINLINE bool IsInterface() const { return Type == EType::CLASS && Class->HasAnyClassFlags(CLASS_Interface) && Class != UInterface::StaticClass(); }

    FORCEINLINE bool IsNative() const { return Struct->IsNative(); }

    FORCEINLINE UStruct* AsStruct() const { return Struct; }

    FORCEINLINE UScriptStruct* AsScriptStruct() const { return Type == EType::SCRIPTSTRUCT ? ScriptStruct : nullptr; }

    FORCEINLINE UClass* AsClass() const { return Type == EType::CLASS ? Class : nullptr; }

    FORCEINLINE FClassDesc* GetParent() const { return Parent; }

    FORCEINLINE const FString& GetName() const { return ClassName; }

    FORCEINLINE FName GetFName() const { return ClassFName; }

    FORCEINLINE const char* GetAnsiName() const { return ClassAnsiName.Get(); }

    FORCEINLINE int32 GetSize() const { return Size; }

    FORCEINLINE uint8 GetUserdataPadding() const { return UserdataPadding; }

    FORCEINLINE int32 GetRefCount() const { return RefCount; }

    FORCEINLINE void AddRef() { ++RefCount; }

    FORCEINLINE FPropertyDesc* GetProperty(int32 Index) { return Index > INDEX_NONE && Index < Properties.Num() ? Properties[Index] : nullptr; }

    FORCEINLINE FFunctionDesc* GetFunction(int32 Index) { return Index > INDEX_NONE && Index < Functions.Num() ? Functions[Index] : nullptr; }

    bool Release(bool bKeepAlive = false);

    void Reset();

    template <typename CharType>
    FFieldDesc* RegisterField(const CharType *FieldName)
    {
        return RegisterField(FName(FieldName), this);
    }

    FFieldDesc* RegisterField(FName FieldName, FClassDesc *QueryClass);

    void GetInheritanceChain(TArray<FString> &NameChain, TArray<UStruct*> &StructChain) const;

    static EType GetType(UStruct *InStruct)
    {
        EType Type = EType::UNKNOWN;
        UScriptStruct *ScriptStruct = Cast<UScriptStruct>(InStruct);
        if (ScriptStruct)
        {
            Type = EType::SCRIPTSTRUCT;
        }
        else
        {
            UClass *Class = Cast<UClass>(InStruct);
            if (Class)
            {
                Type = EType::CLASS;
            }
        }
        return Type;
    }

private:
    union
    {
        UStruct *Struct;
        UScriptStruct *ScriptStruct;
        UClass *Class;
    };

    FString ClassName;
    FName ClassFName;
    TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ClassAnsiName;

    EType Type;
    uint8 UserdataPadding;            // only used for UScriptStruct
    int32 Size;
    int32 RefCount;

    FClassDesc *Parent;
    TArray<FClassDesc*> Interfaces;

    TMap<FName, FFieldDesc*> Fields;
    TArray<FPropertyDesc*> Properties;
    TArray<FFunctionDesc*> Functions;

    struct FFunctionCollection *FunctionCollection;
};

/**
 * Helper class to prevent releasing class descriptors
 */
class FScopedSafeClass
{
public:
    explicit FScopedSafeClass(FClassDesc *InClass)
    {
        AddClass(InClass);
    }

    explicit FScopedSafeClass(const TArray<FClassDesc*> &InClasses)
    {
        for (FClassDesc *InClass : InClasses)
        {
            AddClass(InClass);
        }
    }

    ~FScopedSafeClass()
    {
        for (FClassDesc *Class : Classes)
        {
            Class->Release(true);
        }
    }

private:
    void AddClass(FClassDesc *InClass)
    {
        if (InClass)
        {
            InClass->AddRef();
            Classes.Add(InClass);
        }
    }

    TArray<FClassDesc*> Classes;
};
