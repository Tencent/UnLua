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
#include "LuaEnv.h"
#include "Tickable.h"
#include "UnLuaLatentAction.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FUnLuaLatentActionCallback, int32, InLinkage);

UCLASS()
class UUnLuaLatentAction : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FUnLuaLatentActionCallback Callback;

    UnLua::FLuaEnv* Env;

    UFUNCTION()
    void OnCompleted(int32 InLinkage) const;

    UFUNCTION(BlueprintCallable)
    bool GetTickableWhenPaused();

    UFUNCTION(BlueprintCallable)
    void SetTickableWhenPaused(bool bTickableWhenPaused);

    // Begin Interface FTickableGameObject 

    virtual void Tick(float DeltaTime) override;

    virtual bool IsTickable() const override;

    virtual TStatId GetStatId() const override;

    // End Interface FTickableGameObject

    UFUNCTION()
    void OnLegacyCallback(int32 InLinkage);

private:
    UPROPERTY()
    uint8 bTickEvenWhenPaused:1;
};
