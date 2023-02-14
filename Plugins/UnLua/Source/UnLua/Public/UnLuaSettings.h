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
#include "LuaEnvLocator.h"
#include "LuaModuleLocator.h"
#include "UnLuaSettings.generated.h"


UCLASS(Config=UnLuaSettings, DefaultConfig, Meta=(DisplayName="UnLua"))
class UNLUA_API UUnLuaSettings : public UObject
{
    GENERATED_BODY()

public:
    UUnLuaSettings(const FObjectInitializer& ObjectInitializer);

    /** Entry module name of lua env. Leave it empty to skip execution on startup. */
    UPROPERTY(Config, EditAnywhere, Category="Runtime")
    FString StartupModuleName = TEXT("");

    /** Prevent from infinite loops in lua. Timeout in seconds. */
    UPROPERTY(Config, EditAnywhere, Category="Runtime")
    int32 DeadLoopCheck = 0;

    /** Prevent dangling pointers in lua. */
    UPROPERTY(Config, EditAnywhere, Category="Runtime")
    bool DanglingCheck = false;

    /** Whether to print all Lua env stacks on crash. */
    UPROPERTY(Config, EditAnywhere, Category="Runtime")
    bool bPrintLuaStackOnSystemError = true;

    /** Class of LuaEnvLocator, which handles lua env locating for each UObject. */
    UPROPERTY(Config, EditAnywhere, Category="Runtime", Meta=(AllowAbstract="false"))
    TSubclassOf<ULuaEnvLocator> EnvLocatorClass = ULuaEnvLocator::StaticClass();

    UPROPERTY(Config, EditAnywhere, Category=Runtime, Meta=(AllowAbstract="false", DisplayName="LuaModuleLocator"))
    TSubclassOf<ULuaModuleLocator> ModuleLocatorClass = ULuaModuleLocator::StaticClass();

    /** List of classes to bind on startup. */
    UPROPERTY(config, EditAnywhere, Category=Runtime, meta = (MetaClass="Object", AllowAbstract="True", DisplayName = "List of classes to bind on startup"))
    TArray<FSoftClassPath> PreBindClasses;
};
