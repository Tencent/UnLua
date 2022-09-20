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

#include "Perfs/UnLuaBenchmarkFunctionLibrary.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"

double UUnLuaBenchmarkFunctionLibrary::StartTime;
FString UUnLuaBenchmarkFunctionLibrary::StartTitle;
float UUnLuaBenchmarkFunctionLibrary::BenchmarkMultiplier;
FString UUnLuaBenchmarkFunctionLibrary::BenchmarkTitle;
TArray<FString> UUnLuaBenchmarkFunctionLibrary::Messages;

void UUnLuaBenchmarkFunctionLibrary::Start(const FString& Title, const int32 N)
{
    BenchmarkTitle = Title;
    BenchmarkMultiplier = 1000000000.0f / N;
    Messages.Reset();
    FBlueprintCoreDelegates::SetScriptMaximumLoopIterations(0x7FFFFFFF);
}

void UUnLuaBenchmarkFunctionLibrary::StartTimer(const FString& Title)
{
    StartTime = FPlatformTime::Seconds();
    StartTitle = Title;
}

void UUnLuaBenchmarkFunctionLibrary::StopTimer()
{
    const auto Cost = (FPlatformTime::Seconds() - StartTime) * BenchmarkMultiplier;
    const auto Message = FString::Printf(TEXT("%s ; %f"), *StartTitle, Cost);
    Messages.Add(Message);
}

void UUnLuaBenchmarkFunctionLibrary::Stop()
{
    const auto Message = FString::Join(Messages, TEXT("\n"));
    const auto FilePath = FString::Printf(TEXT("%sBenchmark/%s-Benchmark-%s.csv"), *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), *BenchmarkTitle, *FDateTime::Now().ToString());
    FFileHelper::SaveStringToFile(Message, *FilePath);
}
