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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UnLuaFunctionLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FExecuteWithArgs, const TArray<FString>&, Args);

UCLASS()
class UNLUA_API UUnLuaFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    static FString GetScriptRootPath();

    UFUNCTION(BlueprintCallable)
    static int64 GetFileLastModifiedTimestamp(FString Path);

    UFUNCTION(BlueprintCallable)
    static void HotReload();

    UFUNCTION(BlueprintCallable)
    static void AddConsoleCommand(const FString& Name, const FString& Help, FExecuteWithArgs Execute);

    UFUNCTION(BlueprintCallable)
    static void RemoveConsoleCommand(const FString& Name);

private:
#if ALLOW_CONSOLE
    static TMap<FString, FAutoConsoleCommand*> ConsoleCommands;
#endif
};
