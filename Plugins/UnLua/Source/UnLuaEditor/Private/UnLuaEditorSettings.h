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

public:
    UPROPERTY(config, EditAnywhere, Category = "Coding", meta = (DisplayName = "Hot Reload Mode", defaultValue = 0))
    EHotReloadMode HotReloadMode;

    UPROPERTY(config, EditAnywhere, Category = "Coding", meta = (DisplayName = "Generate IntelliSense Files"))
    bool bGenerateIntelliSense = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Auto Startup", ToolTip = "Requires restart to take effect."))
    bool bAutoStart = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable Debug", ToolTip = "Requires restart to take effect."))
    bool bEnableDebug = false;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable persistent buffer for UFunction's parameters.", ToolTip = "Requires restart to take effect."))
    bool bEnablePersistentParamBuffer = true;
    
    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable Type Checking", ToolTip = "Requires restart to take effect."))
    bool bEnableTypeChecking = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable RPC Call (Deprecated).", ToolTip = "Requires restart to take effect."))
    bool bEnableRPCCall = true;

    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "Enable Call Overridden Function (Deprecated).", ToolTip = "Requires restart to take effect."))
    bool bEnableCallOverriddenFunction = true;
    
    UPROPERTY(config, EditAnywhere, Category = "Build", meta = (DisplayName = "With UE Namespace (Deprecated)", ToolTip = "Requires restart to take effect."))
    bool bWithUENamespace = true;

    UPROPERTY(config, EditAnywhere, Category = "Legacy", meta = (DisplayName = "Place out parameters before return value", ToolTip = "Requires restart to take effect."))
    bool bLegacyReturnOrder = false;

    UPROPERTY(config, EditAnywhere, Category = "Legacy", meta = (DisplayName = "Auto append '_C' to blueprint class path", ToolTip = "Requires restart to take effect."))
    bool bLegacyBlueprintPath = false;

    UPROPERTY(config, EditAnywhere, Category = "System", meta = (DisplayName = "Update Mode", defaultValue = 0))
    EUpdateMode UpdateMode;
};
