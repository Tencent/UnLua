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

class FUnLuaIntelliSenseGenerator
{
public:
    FUnLuaIntelliSenseGenerator()
        : bInitialized(false)
    {
    }

    static TSharedRef<FUnLuaIntelliSenseGenerator> Get();
    
    // Update all Blueprints from command
    void UpdateAll();

    void Initialize();

private:
    static TSharedPtr<FUnLuaIntelliSenseGenerator> Singleton;

    static bool IsBlueprint(const FAssetData& AssetData);

    static bool ShouldExport(const FAssetData& AssetData);

    void Export(const UBlueprint* Blueprint);

    void Export(const UField* Field);

    void ExportUE(const TArray<const UField*> Types);

    void CollectTypes(TArray<const UField*> &Types);
    
    // File helper
    void SaveFile(const FString& ModuleName, const FString& FileName, const FString& GeneratedFileContent);
    void DeleteFile(const FString& ModuleName, const FString& FileName);

    // Handle asset event
    void OnAssetAdded(const FAssetData& AssetData);
    void OnAssetRemoved(const FAssetData& AssetData);
    void OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath);
    void OnAssetUpdated(const FAssetData& AssetData);

    FString OutputDir;
    bool bInitialized;
};
