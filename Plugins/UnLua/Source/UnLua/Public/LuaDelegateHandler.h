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
#include "UObject/Object.h"
#include "LuaDelegateHandler.generated.h"

namespace UnLua
{
    class FLuaEnv;
}

UCLASS()
class UNLUA_API ULuaDelegateHandler : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void Dummy();

    bool IsAlive() const;

    void BindTo(FDelegateProperty* InProperty, FScriptDelegate* InDelegate);

    void AddTo(FMulticastDelegateProperty* InProperty, void* InDelegate);

    virtual void ProcessEvent(UFunction* Function, void* Parms) override;

    void RemoveFrom(FMulticastDelegateProperty* InProperty, void* InDelegate);

    static ULuaDelegateHandler* CreateFrom(UnLua::FLuaEnv* InEnv, int32 InLuaRef, UObject* InLifecycle);

    TWeakObjectPtr<UObject> Lifecycle;
    UnLua::FLuaEnv* Env;
    int32 LuaRef;
    FProperty* Property;
    void* Delegate;
};
