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

#include "UnLuaInterface.h"
#include "IssueOverridesTest.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UNLUATESTSUITE_API AIssueOverridesActor : public AActor
{
    GENERATED_BODY()
};

UCLASS()
class UNLUATESTSUITE_API UIssueOverridesObject
    : public UObject,
      public IUnLuaInterface
{
    GENERATED_BODY()

public:
    virtual FString GetModuleName_Implementation() const override
    {
        return TEXT("Tests.Regression.IssueOverrides.IssueOverridesObject");
    }
    
    UFUNCTION(BlueprintCallable)
    int32 GetConfig() const { return 1; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    int32 CollectInfo() const;
};

inline int32 UIssueOverridesObject::CollectInfo_Implementation() const
{
    UE_LOG(LogTemp, Log, TEXT("CollectInfo C++"));
    return 1;
}
