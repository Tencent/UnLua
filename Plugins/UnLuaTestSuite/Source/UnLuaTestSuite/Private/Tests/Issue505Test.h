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

#include "UnLuaTestHelpers.h"
#include "Issue505Test.generated.h"

USTRUCT(BlueprintType)
struct UNLUATESTSUITE_API FUnLuaIssue505Struct
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    TArray<FUnLuaTestTableRow> Array;

    FUnLuaIssue505Struct()
    {
        FUnLuaTestTableRow Row1;
        Row1.Level = 1;
        Row1.Title = "Row1";
        Array.Add(Row1);

        FUnLuaTestTableRow Row2;
        Row2.Level = 2;
        Row2.Title = "Row2";
        Array.Add(Row2);
    }
};

UCLASS(BlueprintType)
class UUnLuaIssue505Helper : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static FUnLuaIssue505Struct GetStruct()
    {
        return FUnLuaIssue505Struct();
    }
};
