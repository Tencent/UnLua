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
#include "Issue566Test.generated.h"

DECLARE_DYNAMIC_DELEGATE(FIssue566Delegate);

UCLASS()
class UIssue566FunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void AddCallback(FIssue566Delegate InDelegate)
    {
        Delegates.Add(InDelegate);
    }

    UFUNCTION(BlueprintCallable)
    static void Invoke()
    {
        for (auto& Delegate : Delegates)
        {
            if (Delegate.IsBound())
                Delegate.Execute();
        }
    }

    UFUNCTION(BlueprintCallable)
    static void Reset()
    {
        Delegates.Empty();
    }

private:
    static TArray<FIssue566Delegate> Delegates;
};

UCLASS()
class UIssue566Object : public UObject
{
    GENERATED_BODY()
};
