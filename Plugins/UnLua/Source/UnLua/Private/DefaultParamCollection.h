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

#include "CoreMinimal.h"

class IParamValue
{
public:
    virtual ~IParamValue()
    {
    }

    virtual const void* GetValue() = 0;
};

template <typename T>
class TParamValue : public IParamValue
{
public:
    explicit TParamValue(const T& InValue)
        : Value(InValue)
    {
    }

    virtual const void* GetValue() override { return &Value; }

private:
    T Value;
};

class FRuntimeEnumParamValue : public IParamValue
{
public:
    explicit FRuntimeEnumParamValue(const FString& InCppType, int32 InIndex)
        : bInitialized(false), TypeName(InCppType), Index(InIndex)
    {
    }

    virtual const void* GetValue() override
    {
        if (!bInitialized)
        {
            UEnum* Enum = FindObjectChecked<UEnum>(ANY_PACKAGE, *TypeName);
            Value = Enum->GetValueByIndex(Index);
            bInitialized = true;
        }
        return &Value;
    }

private:
    bool bInitialized;
    FString TypeName;
    int32 Index;
    int64 Value;
};

class FScriptArrayParamValue : public IParamValue
{
public:
    explicit FScriptArrayParamValue()
    {
    }

    virtual const void* GetValue() override { return &Value; }

private:
    FScriptArray Value;
};

typedef TParamValue<bool> FBoolParamValue;
typedef TParamValue<uint8> FByteParamValue;
typedef TParamValue<int32> FIntParamValue;
typedef TParamValue<int64> FEnumParamValue;
typedef TParamValue<float> FFloatParamValue;
typedef TParamValue<double> FDoubleParamValue;
typedef TParamValue<FName> FNameParamValue;
typedef TParamValue<FText> FTextParamValue;
typedef TParamValue<FString> FStringParamValue;
typedef TParamValue<FVector> FVectorParamValue;
typedef TParamValue<FVector2D> FVector2DParamValue;
typedef TParamValue<FRotator> FRotatorParamValue;
typedef TParamValue<FLinearColor> FLinearColorParamValue;
typedef TParamValue<FColor> FColorParamValue;
typedef TParamValue<FScriptDelegate> FScriptDelegateParamValue;
typedef TParamValue<FMulticastScriptDelegate> FMulticastScriptDelegateParamValue;

struct FParameterCollection
{
    TMap<FName, IParamValue*> Parameters;
};

struct FFunctionCollection
{
    TMap<FName, FParameterCollection> Functions;
};

extern TMap<FName, FFunctionCollection> GDefaultParamCollection;

extern void CreateDefaultParamCollection();
