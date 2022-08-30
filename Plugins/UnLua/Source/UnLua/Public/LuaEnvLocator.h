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
#include "Engine/GameInstance.h"
#include "LuaEnv.h"
#include "LuaEnvLocator.generated.h"

UCLASS()
class UNLUA_API ULuaEnvLocator : public UObject
{
    GENERATED_BODY()
public:
    virtual UnLua::FLuaEnv* Locate(const UObject* Object);

    virtual void HotReload();

    virtual void Reset();

    TSharedPtr<UnLua::FLuaEnv, ESPMode::ThreadSafe> Env;
};

UCLASS()
class UNLUA_API ULuaEnvLocator_ByGameInstance : public ULuaEnvLocator
{
    GENERATED_BODY()
public:
    virtual UnLua::FLuaEnv* Locate(const UObject* Object) override;

    virtual void HotReload() override;

    virtual void Reset() override;

    UnLua::FLuaEnv* GetDefault();

    TMap<TWeakObjectPtr<UGameInstance>, TSharedPtr<UnLua::FLuaEnv, ESPMode::ThreadSafe>> Envs;
};
