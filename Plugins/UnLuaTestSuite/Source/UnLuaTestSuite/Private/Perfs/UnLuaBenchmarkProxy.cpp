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

#include "Perfs/UnLuaBenchmarkProxy.h"

void AUnLuaBenchmarkProxy::NOP()
{
}

void AUnLuaBenchmarkProxy::Simulate(float DeltaTime)
{
}

int32 AUnLuaBenchmarkProxy::GetMeshID() const
{
    return MeshID;
}

const FString& AUnLuaBenchmarkProxy::GetMeshName() const
{
    return MeshName;
}

const FVector& AUnLuaBenchmarkProxy::GetCOM() const
{
    return COM;
}

int32 AUnLuaBenchmarkProxy::UpdateMeshID(int32 NewID)
{
    return NewID;
}

FString AUnLuaBenchmarkProxy::UpdateMeshName(const FString &NewName)
{
    return NewName;
}

bool AUnLuaBenchmarkProxy::Raycast(const FVector &Origin, const FVector &Direction) const
{
    return true;
}

void AUnLuaBenchmarkProxy::GetIndices(TArray<int32> &OutIndices) const
{
}

void AUnLuaBenchmarkProxy::UpdateIndices(const TArray<int32> &NewIndices)
{
}

void AUnLuaBenchmarkProxy::GetPositions(TArray<FVector> &OutPositions) const
{
}

void AUnLuaBenchmarkProxy::UpdatePositions(const TArray<FVector> &NewPositions)
{
}

const TArray<FVector>& AUnLuaBenchmarkProxy::GetPredictedPositions() const
{
    return PredictedPositions;
}

bool AUnLuaBenchmarkProxy::GetMeshInfo(int32 &OutMeshID, FString &OutMeshName, FVector &OutCOM, TArray<int32> &OutIndices, TArray<FVector> &OutPositions, TArray<FVector> &OutPredictedPositions) const
{
    return true;
}
