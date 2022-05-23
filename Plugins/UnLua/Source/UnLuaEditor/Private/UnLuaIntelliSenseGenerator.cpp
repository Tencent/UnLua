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

#include "UnLuaIntelliSenseGenerator.h"
#include "AssetRegistryModule.h"
#include "CoreUObject.h"
#include "UnLua.h"
#include "UnLuaEditorSettings.h"
#include "UnLuaIntelliSense.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "UnLuaIntelliSenseGenerator"

TSharedPtr<FUnLuaIntelliSenseGenerator> FUnLuaIntelliSenseGenerator::Singleton;

TSharedRef<FUnLuaIntelliSenseGenerator> FUnLuaIntelliSenseGenerator::Get()
{
    if (!Singleton.IsValid())
        Singleton = MakeShareable(new FUnLuaIntelliSenseGenerator);
    return Singleton.ToSharedRef();
}

void FUnLuaIntelliSenseGenerator::Initialize()
{
    if (bInitialized)
        return;

    OutputDir = IPluginManager::Get().FindPlugin("UnLua")->GetBaseDir() + "/Intermediate/";

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FUnLuaIntelliSenseGenerator::OnAssetAdded);
    AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FUnLuaIntelliSenseGenerator::OnAssetRemoved);
    AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FUnLuaIntelliSenseGenerator::OnAssetRenamed);
    AssetRegistryModule.Get().OnAssetUpdated().AddRaw(this, &FUnLuaIntelliSenseGenerator::OnAssetUpdated);

    bInitialized = true;
}

void FUnLuaIntelliSenseGenerator::UpdateAll()
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

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

    TArray<const UField*> Types;
    CollectTypes(Types);
    for (const auto Type : Types)
        Export(Type);

    ExportUE(Types);
}

bool FUnLuaIntelliSenseGenerator::IsBlueprint(const FAssetData& AssetData)
{
    const FName AssetClass = AssetData.AssetClass;
    return AssetClass == UBlueprint::StaticClass()->GetFName() || AssetClass == UWidgetBlueprint::StaticClass()->GetFName();
}

bool FUnLuaIntelliSenseGenerator::ShouldExport(const FAssetData& AssetData)
{
    const auto& Settings = *GetDefault<UUnLuaEditorSettings>();
    if (!Settings.bGenerateIntelliSense)
        return false;

    if (!IsBlueprint(AssetData))
        return false;

	UObject* tUobj = AssetData.FastGetAsset();
	if (tUobj)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(tUobj))
		{
			if (Blueprint->SkeletonGeneratedClass || Blueprint->GeneratedClass)
			{
				return true;
			}
		}
	}

    return false;
}

void FUnLuaIntelliSenseGenerator::Export(const UBlueprint* Blueprint)
{
    const FString BPName = Blueprint->GetName();
    const FString Content = UnLua::IntelliSense::Get(Blueprint);
    SaveFile("/Game", BPName, Content);
}

void FUnLuaIntelliSenseGenerator::Export(const UField* Field)
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    const UPackage* Package = Field->GetPackage();
#else
    const UPackage* Package = (UPackage*)Field->GetTypedOuter(UPackage::StaticClass());
#endif
    const FString ModuleName = Package->GetName();
    const FString FileName = UnLua::IntelliSense::GetTypeName(Field);
    const FString Content = UnLua::IntelliSense::Get(Field);
    SaveFile(ModuleName, FileName, Content);
}

void FUnLuaIntelliSenseGenerator::ExportUE(const TArray<const UField*> Types)
{
    const FString Content = UnLua::IntelliSense::GetUE(Types);
    SaveFile("", "UE", Content);
}

void FUnLuaIntelliSenseGenerator::CollectTypes(TArray<const UField*>& Types)
{
    for (TObjectIterator<UClass> It; It; ++It)
    {
        const UClass* Class = *It;
        const FString ClassName = Class->GetName();
        // ReSharper disable StringLiteralTypo
        if (ClassName.StartsWith("SKEL_")
            || ClassName.StartsWith("PLACEHOLDER-CLASS")
            || ClassName.StartsWith("REINST_")
            || ClassName.StartsWith("TRASHCLASS_")
            || ClassName.StartsWith("HOTRELOADED_")
        )
        {
            // skip nonsense types
            continue;
        }
        // ReSharper restore StringLiteralTypo
        Types.Add(Class);
    }

    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        const UScriptStruct* ScriptStruct = *It;
        Types.Add(ScriptStruct);
    }

    for (TObjectIterator<UEnum> It; It; ++It)
    {
        const UEnum* Enum = *It;
        Types.Add(Enum);
    }
}

void FUnLuaIntelliSenseGenerator::SaveFile(const FString& ModuleName, const FString& FileName, const FString& GeneratedFileContent)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString Directory = FString::Printf(TEXT("%sIntelliSense%s"), *OutputDir, *ModuleName);
    if (!FileManager.DirectoryExists(*Directory))
        FileManager.MakeDirectory(*Directory);

    const FString FilePath = FString::Printf(TEXT("%s/%s.lua"), *Directory, *FileName);
    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePath);
    if (FileContent != GeneratedFileContent)
        FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
}

void FUnLuaIntelliSenseGenerator::DeleteFile(const FString& ModuleName, const FString& FileName)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString Directory = FString::Printf(TEXT("%sIntelliSense/%s"), *OutputDir, *ModuleName);
    if (!FileManager.DirectoryExists(*Directory))
        FileManager.MakeDirectory(*Directory);

    const FString FilePath = FString::Printf(TEXT("%s/%s.lua"), *Directory, *FileName);
    if (FileManager.FileExists(*FilePath))
        FileManager.Delete(*FilePath);
}

void FUnLuaIntelliSenseGenerator::OnAssetAdded(const FAssetData& AssetData)
{
    if (!ShouldExport(AssetData))
        return;
    
    OnAssetUpdated(AssetData);

    TArray<const UField*> Types;
    CollectTypes(Types);
    ExportUE(Types);
}

void FUnLuaIntelliSenseGenerator::OnAssetRemoved(const FAssetData& AssetData)
{
    if (!ShouldExport(AssetData))
        return;

    DeleteFile(FString("/Game"), AssetData.AssetName.ToString());
}

void FUnLuaIntelliSenseGenerator::OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
    if (!ShouldExport(AssetData))
        return;

    //remove old Blueprint name
    const FString OldPackageName = FPackageName::GetShortName(OldPath);
    DeleteFile("/Game", OldPackageName);

    //update new name 
    OnAssetUpdated(AssetData);
}

void FUnLuaIntelliSenseGenerator::OnAssetUpdated(const FAssetData& AssetData)
{
    if (!ShouldExport(AssetData))
        return;

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetData.ObjectPath.ToString());
    if (!Blueprint)
        return;

    Export(Blueprint);
}
