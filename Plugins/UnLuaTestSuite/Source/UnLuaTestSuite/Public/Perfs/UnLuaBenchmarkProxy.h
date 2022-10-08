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

#include "GameFramework/Actor.h"
#include "UnLuaBenchmarkProxy.generated.h"

UCLASS()
class AUnLuaBenchmarkProxy : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void NOP();

    UFUNCTION(BlueprintCallable)
    void Simulate(float DeltaTime);

    UFUNCTION(BlueprintCallable)
    int32 GetMeshID() const;

    UFUNCTION(BlueprintCallable)
    const FString& GetMeshName() const;

    UFUNCTION(BlueprintCallable)
    const FVector& GetCOM() const;

    UFUNCTION(BlueprintCallable)
    int32 UpdateMeshID(int32 NewID);

    UFUNCTION(BlueprintCallable)
    FString UpdateMeshName(const FString &NewName);

    UFUNCTION(BlueprintCallable)
    bool Raycast(const FVector &Origin, const FVector &Direction) const;

    UFUNCTION(BlueprintCallable)
    void GetIndices(TArray<int32> &OutIndices) const;

    UFUNCTION(BlueprintCallable)
    void UpdateIndices(const TArray<int32> &NewIndices);

    UFUNCTION(BlueprintCallable)
    void GetPositions(TArray<FVector> &OutPositions) const;

    UFUNCTION(BlueprintCallable)
    void UpdatePositions(const TArray<FVector> &NewPositions);

    UFUNCTION(BlueprintCallable)
    const TArray<FVector>& GetPredictedPositions() const;

    UFUNCTION(BlueprintCallable)
    bool GetMeshInfo(int32 &OutMeshID, FString &OutMeshName, FVector &OutCOM, TArray<int32> &OutIndices, TArray<FVector> &OutPositions, TArray<FVector> &OutPredictedPositions) const;

    UPROPERTY(BlueprintReadWrite)
    int32 MeshID;

    UPROPERTY(BlueprintReadWrite)
    FString MeshName;

    UPROPERTY(BlueprintReadWrite)
    FVector COM;

    UPROPERTY(BlueprintReadWrite)
    TArray<int32> Indices;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector> Positions;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVector> PredictedPositions;
};
