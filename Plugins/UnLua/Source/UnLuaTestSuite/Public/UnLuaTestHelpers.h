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
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UnLua.h"
#include "UnLuaTestHelpers.generated.h"

namespace UnLuaTestHelpers
{
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUnLuaTestSimpleEvent);

DECLARE_DYNAMIC_DELEGATE(FUnLuaTestSimpleHandler);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIssue304Event, TArray<FString>, Array);

UCLASS()
class UNLUATESTSUITE_API UUnLuaTestStub : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FUnLuaTestSimpleEvent SimpleEvent;

    UPROPERTY()
    FUnLuaTestSimpleHandler SimpleHandler;

    UPROPERTY()
    FIssue304Event Issue304Event;

    UPROPERTY()
    int32 Counter;

    UFUNCTION(BlueprintCallable)
    void AddCount() { Counter++; }
};

UCLASS()
class UNLUATESTSUITE_API AUnLuaTestActor : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    int32 TestForIssue300();
};

USTRUCT(BlueprintType)
struct UNLUATESTSUITE_API FUnLuaTestTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FIssule294Event, int32, Value1, UObject*, Value2);

UCLASS()
class UNLUATESTSUITE_API UUnLuaTestFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    UFUNCTION(BlueprintCallable)
    static int32 TestForIssue293(const FString& A, int32 B, const TArray<FColor>& C) { return C.Num(); }

    UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Event, Array"))
    static int32 TestForIssue294(const FString& A, int32 B, const FIssule294Event& Event, const TArray<FColor>& Array)
    {
        Event.ExecuteIfBound(1, StaticClass());
        return Array.Num();
    }
};

#if WITH_DEV_AUTOMATION_TESTS

#define TEST_TRUE(expression) \
EPIC_TEST_BOOLEAN_(TEXT(#expression), expression, true)

#define TEST_FALSE(expression) \
EPIC_TEST_BOOLEAN_(TEXT(#expression), expression, false)

#define TEST_EQUAL(expression, expected) \
EPIC_TEST_BOOLEAN_(TEXT(#expression), expression, expected)

#define EPIC_TEST_BOOLEAN_(text, expression, expected) \
TestEqual(text, expression, expected);

#endif //WITH_DEV_AUTOMATION_TESTS
