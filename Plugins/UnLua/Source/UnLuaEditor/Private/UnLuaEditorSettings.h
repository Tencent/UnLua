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

    UPROPERTY(config, EditAnywhere, Category = "Coding", meta = (DisplayName = "Hot Reload Mode", defaultValue = 0))
    EHotReloadMode HotReloadMode;

    UPROPERTY(config, EditAnywhere, Category = "Coding", meta = (DisplayName = "Generate IntelliSense Files"))
    bool bGenerateIntelliSense = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Auto Startup"))
    bool bAutoStart = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable Debug"))
    bool bEnableDebug = false;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable Type Checking"))
    bool bEnableTypeChecking = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable RPC Call (Deprecated)"))
    bool bEnableRPCCall = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "With UE Namespace (Deprecated)"))
    bool bWithUENamespace = true;

    UPROPERTY(config, EditAnywhere, Category = "System", meta = (DisplayName = "Update Mode", defaultValue = 0))
    EUpdateMode UpdateMode;
};
