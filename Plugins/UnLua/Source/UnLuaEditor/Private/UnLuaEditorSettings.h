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
#include "UnLuaEditorSettings.generated.h"

#define LOCTEXT_NAMESPACE "UnLuaEditorSettings"

UENUM()
enum class EUpdateMode : uint8
{
    Start,
    Manual,
    None,
};

UENUM()
enum class EHotReloadMode : uint8
{
    Manual,
    Auto,
    Never
};

UCLASS(config=UnLuaEditor, defaultconfig, meta=(DisplayName="UnLuaEditor"))
class UNLUAEDITOR_API UUnLuaEditorSettings : public UObject
{
    GENERATED_BODY()

public:
    /** Pick a method for lua hot-reload. */
    UPROPERTY(config, EditAnywhere, Category = "Coding", meta = (defaultValue = 0))
    EHotReloadMode HotReloadMode;

    /** Whether or not generate intellisense files for lua. */
    UPROPERTY(config, EditAnywhere, Category = "Coding")
    bool bGenerateIntelliSense = true;

    /** Whether or not startup UnLua module on game start. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bAutoStartup = true;

    /** Enable UnLua debug routine codes. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnableDebug = false;

    /** Enable Unreal Insights to profile call performance. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnableUnrealInsights = false;

    /** Enable persistent buffer for UFunction's parameters. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnablePersistentParamBuffer = true;

    /** Enable type checking at lua runtime. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnableTypeChecking = true;

    /** Enable 'Overridden' support at lua runtime. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnableCallOverriddenFunction = true;

    /** Enable FText support at lua runtime which will no longer be treated as a string. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bEnableFText = true;

    /** Whether or not compile lua module as c++ code. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    bool bLuaCompileAsCpp = false;

    /** Use the specified lua version. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Build")
    FString LuaVersion = TEXT("lua-5.4.3");

    /** Create UE4 global table on lua env. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Legacy")
    bool bWithUE4Namespace = true;

    /** Place out parameters before return value. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Legacy")
    bool bLegacyReturnOrder = false;

    /** Auto append '_C' to blueprint class path. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Legacy")
    bool bLegacyBlueprintPath = false;

    /** Allow lua file with UTF-8 BOM header. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Legacy")
    bool bLegacyAllowUTF8WithBOM = false;

    /** Arguments are passed to lua by pointer when called from UE. (Requires restart to take effect) */
    UPROPERTY(config, EditAnywhere, Category = "Legacy")
    bool bLegacyArgsPassing = true;

    UPROPERTY(config, EditAnywhere, Category = "System", meta = (defaultValue = 0))
    EUpdateMode UpdateMode;
};

#undef LOCTEXT_NAMESPACE
