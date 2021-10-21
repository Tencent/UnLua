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

#include "UnLua.h"
#include "TestHelpers.generated.h"

namespace UnLuaTestHelpers
{
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUnLuaTestSimpleEvent);

DECLARE_DYNAMIC_DELEGATE(FUnLuaTestSimpleHandler);

UCLASS()
class UNLUA_API UUnLuaTestStub : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FUnLuaTestSimpleEvent SimpleEvent;

    UPROPERTY()
    FUnLuaTestSimpleHandler SimpleHandler;

    UPROPERTY()
    int32 Counter;

    UFUNCTION(BlueprintCallable)
    void AddCount() { Counter++; }
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
