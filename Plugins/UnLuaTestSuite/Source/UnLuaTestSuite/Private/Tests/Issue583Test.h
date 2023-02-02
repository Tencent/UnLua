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

#include "Engine/DataTable.h"
#include "Issue583Test.generated.h"

USTRUCT(BlueprintType)
struct FIssue583Record
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FText Description;

    UPROPERTY(EditAnywhere)
    int32 Flag = 200;
};

USTRUCT(BlueprintType)
struct FIssue583Row : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TMap<FString, FIssue583Record> Records;
};
