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
    virtual ~IParamValue() {}

    virtual const void* GetValue() const = 0;
};

template <typename T>
class TParamValue : public IParamValue
{
public:
    explicit TParamValue(const T &InValue)
        : Value(InValue)
    {}

    virtual const void* GetValue() const override { return &Value; }

private:
    T Value;
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
extern void DestroyDefaultParamCollection();
