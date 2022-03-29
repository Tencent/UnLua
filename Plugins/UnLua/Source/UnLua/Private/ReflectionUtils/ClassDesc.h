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
public:
    FClassDesc(UStruct *InStruct, const FString &InName);
    ~FClassDesc();

    FORCEINLINE bool IsValid() const { return true; }

    FORCEINLINE bool IsScriptStruct() const { return bIsScriptStruct; }

    FORCEINLINE bool IsClass() const { return bIsClass; }

    FORCEINLINE bool IsInterface() const { return bIsInterface; }

    FORCEINLINE bool IsNative() const { return bIsInterface; }

    FORCEINLINE UStruct* AsStruct()
    {
        Load();
        return Struct;
    }

    FORCEINLINE UScriptStruct* AsScriptStruct()
    {
        if (!bIsScriptStruct)
            return nullptr;
        Load();
        return static_cast<UScriptStruct*>(Struct);
    }

    FORCEINLINE UClass* AsClass()
    {
        if (!bIsClass)
            return nullptr;
        Load();
        return static_cast<UClass*>(Struct);
    }

    FORCEINLINE FString GetName() const { return ClassName; }

    FORCEINLINE int32 GetSize() const { return Size; }

    FORCEINLINE uint8 GetUserdataPadding() const { return UserdataPadding; }

    FORCEINLINE int32 GetRefCount() const { return RefCount; }

    FORCEINLINE FPropertyDesc* GetProperty(int32 Index) { return Index > INDEX_NONE && Index < Properties.Num() ? Properties[Index] : nullptr; }

    FORCEINLINE FFunctionDesc* GetFunction(int32 Index) { return Index > INDEX_NONE && Index < Functions.Num() ? Functions[Index] : nullptr; }

    void AddRef();

    void SubRef();

    FFieldDesc* FindField(const char* FieldName);

    FFieldDesc* RegisterField(FName FieldName, FClassDesc *QueryClass = nullptr);

    void GetInheritanceChain(TArray<FClassDesc*>& Chain);

    void Load();
    
    void UnLoad();

private:
    UStruct* Struct;

    FString ClassName;

    uint8 bIsScriptStruct : 1;
    uint8 bIsClass : 1;
    uint8 bIsInterface : 1;
    uint8 bIsNative : 1;

    int32 UserdataPadding : 8;            // only used for UScriptStruct
    int32 Size : 24;
    int32 RefCount;

    TArray<FClassDesc*> Interfaces;
    TMap<FName, FFieldDesc*> Fields;
    TArray<FPropertyDesc*> Properties;
    TArray<FFunctionDesc*> Functions;
    TArray<FClassDesc*> SuperClasses;

    struct FFunctionCollection *FunctionCollection;
};
