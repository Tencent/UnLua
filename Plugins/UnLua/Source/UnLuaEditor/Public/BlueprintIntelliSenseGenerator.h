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

#include "CoreUObject.h"

class UBlueprint;
class UWidgetBlueprint;

class FBlueprintIntelliSenseGenerator
{
public:
    FBlueprintIntelliSenseGenerator()
        : bInitialized(false)
    {
    }

    static TSharedRef<FBlueprintIntelliSenseGenerator> Get();

    //Update all Blueprints from command
    void UpdateAll();

    void Initialize();

private:
    static TSharedPtr<FBlueprintIntelliSenseGenerator> Singleton;

    //Export bp variables
    void ExportBPFunctions(const UBlueprint* Blueprint);
    void ExportBPSyntax(const UBlueprint* Blueprint);
    void ExportWidgetBPSyntax(const UWidgetBlueprint* WidgetBlueprint);
    void ExportSyntaxToFile(const FString& BPName);

    //File helper
    void SaveFile(const FString& ModuleName, const FString& FileName, const FString& GeneratedFileContent);
    void DeleteFile(const FString& ModuleName, const FString& FileName);
    void CatalogInit();
    void CatalogAdd(const FString& BPName);
    void CatalogRemove(const FString& BPName);
    void CatalogFileUpdate();

    //Handle asset event
    void OnAssetAdded(const FAssetData& AssetData);
    void OnAssetRemoved(const FAssetData& AssetData);
    void OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath);
    void OnAssetUpdated(const FAssetData& AssetData);

    // Get function IntelliSense
    FString GetFunctionStr(const UFunction* Function, FString ClassName) const;

    // Get function properties string
    FString GetFunctionProperties(const UFunction* Function, FString& Properties) const;

    // Get readable type name for a UPROPERTY
    FString GetTypeName(const FProperty* Property) const;

    TMap<FString, TArray<FString>> BPVariablesMap;
    TMap<FString, TArray<FString>> BPFunctionMap;
    FString OutputDir;
    FString CatalogContent;
    bool bInitialized;
};
