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

#include "BlueprintIntelliSenseGenerator.h"
#include "AssetRegistryModule.h"
#include "CoreUObject.h"
#include "ToolMenus.h"
#include "UnLua.h"
#include "UnLuaIntelliSense.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "BlueprintIntelliSenseGenerator"

TSharedPtr<FBlueprintIntelliSenseGenerator> FBlueprintIntelliSenseGenerator::Singleton;

TSharedRef<FBlueprintIntelliSenseGenerator> FBlueprintIntelliSenseGenerator::Get()
{
    if (!Singleton.IsValid())
        Singleton = MakeShareable(new FBlueprintIntelliSenseGenerator);
    return Singleton.ToSharedRef();
}

void FBlueprintIntelliSenseGenerator::Initialize()
{
    if (bInitialized)
        return;

    OutputDir = IPluginManager::Get().FindPlugin("UnLua")->GetBaseDir() + "/Intermediate/";

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FBlueprintIntelliSenseGenerator::OnAssetAdded);
    AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FBlueprintIntelliSenseGenerator::OnAssetRemoved);
    AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FBlueprintIntelliSenseGenerator::OnAssetRenamed);
    AssetRegistryModule.Get().OnAssetUpdated().AddRaw(this, &FBlueprintIntelliSenseGenerator::OnAssetUpdated);

    bInitialized = true;
}

void FBlueprintIntelliSenseGenerator::UpdateAll()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    FARFilter Filter;
    Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
    Filter.ClassNames.Add(UWidgetBlueprint::StaticClass()->GetFName());

    TArray<FAssetData> BlueprintList;

    if (AssetRegistryModule.Get().GetAssets(Filter, BlueprintList))
    {
        FScopedSlowTask SlowTask(BlueprintList.Num(), LOCTEXT("GeneratingBlueprintsIntelliSense", "Generating Blueprints InstelliSense"));
        SlowTask.MakeDialog();

        for (int32 i = 0; i < BlueprintList.Num(); i++)
        {
            if (SlowTask.ShouldCancel())
                break;
            OnAssetUpdated(BlueprintList[i]);
            SlowTask.EnterProgressFrame();
        }
    }
}

bool FBlueprintIntelliSenseGenerator::IsBlueprint(const FAssetData& AssetData)
{
    const FName AssetClass = AssetData.AssetClass;
    return AssetClass == UBlueprint::StaticClass()->GetFName() || AssetClass == UWidgetBlueprint::StaticClass()->GetFName();
}

void FBlueprintIntelliSenseGenerator::Export(const UBlueprint* Blueprint)
{
    FString BPName = Blueprint->GetName();
    FString Content = UnLua::IntelliSense::Get(Blueprint);
    SaveFile(FString("Blueprints"), BPName, Content);
}

void FBlueprintIntelliSenseGenerator::SaveFile(const FString& ModuleName, const FString& FileName, const FString& GeneratedFileContent)
{
    IFileManager& FileManager = IFileManager::Get();
    FString Directory = FString::Printf(TEXT("%sIntelliSense/%s"), *OutputDir, *ModuleName);
    if (!FileManager.DirectoryExists(*Directory))
    {
        FileManager.MakeDirectory(*Directory);
    }

    const FString FilePath = FString::Printf(TEXT("%s/%s.lua"), *Directory, *FileName);
    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePath);
    if (FileContent != GeneratedFileContent)
    {
        bool bResult = FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
        //check(bResult);
    }
}

void FBlueprintIntelliSenseGenerator::DeleteFile(const FString& ModuleName, const FString& FileName)
{
    IFileManager& FileManager = IFileManager::Get();
    FString Directory = FString::Printf(TEXT("%sIntelliSense/%s"), *OutputDir, *ModuleName);
    if (!FileManager.DirectoryExists(*Directory))
    {
        FileManager.MakeDirectory(*Directory);
    }
    const FString FilePath = FString::Printf(TEXT("%s/%s.lua"), *Directory, *FileName);
    if (FileManager.FileExists(*FilePath))
    {
        FileManager.Delete(*FilePath);
    }
}

void FBlueprintIntelliSenseGenerator::OnAssetAdded(const FAssetData& AssetData)
{
    OnAssetUpdated(AssetData);
}

void FBlueprintIntelliSenseGenerator::OnAssetRemoved(const FAssetData& AssetData)
{
    if (!IsBlueprint(AssetData))
        return;

    DeleteFile(FString("Blueprints"), AssetData.AssetName.ToString());
}

void FBlueprintIntelliSenseGenerator::OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
    if (!IsBlueprint(AssetData))
        return;

    //remove old Blueprint name
    const FString OldPackageName = FPackageName::GetShortName(OldPath);
    DeleteFile(FString("Blueprints"), OldPackageName);

    //update new name 
    OnAssetUpdated(AssetData);
}

void FBlueprintIntelliSenseGenerator::OnAssetUpdated(const FAssetData& AssetData)
{
    if (!IsBlueprint(AssetData))
        return;

    //UE_LOG(LogTemp, Warning, TEXT("updated bp name is %s"), *AssetData.GetFullName());
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetData.ObjectPath.ToString());
    if (!Blueprint)
        return;

    Export(Blueprint);
}
