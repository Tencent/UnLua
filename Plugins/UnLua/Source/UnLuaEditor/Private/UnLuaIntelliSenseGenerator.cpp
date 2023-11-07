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

#include "Misc/EngineVersionComparison.h"
#include "UnLuaIntelliSenseGenerator.h"
#if UE_VERSION_NEWER_THAN(5, 1, 0)
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
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

    OutputDir = IPluginManager::Get().FindPlugin("UnLua")->GetBaseDir() + "/Intermediate/IntelliSense";

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
#if UE_VERSION_OLDER_THAN(5, 1, 0)
    Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
    Filter.ClassNames.Add(UWidgetBlueprint::StaticClass()->GetFName());
#else
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
#endif

    TArray<FAssetData> BlueprintAssets;
    TArray<const UField*> NativeTypes;
    AssetRegistryModule.Get().GetAssets(Filter, BlueprintAssets);
    CollectTypes(NativeTypes);

    auto TotalCount = BlueprintAssets.Num() + NativeTypes.Num();
    if (TotalCount == 0)
        return;

    TotalCount++;

    FScopedSlowTask SlowTask(TotalCount, LOCTEXT("GeneratingBlueprintsIntelliSense", "Generating Blueprints InstelliSense"));
    SlowTask.MakeDialog();

    for (int32 i = 0; i < BlueprintAssets.Num(); i++)
    {
        if (SlowTask.ShouldCancel())
            break;
        OnAssetUpdated(BlueprintAssets[i]);
        SlowTask.EnterProgressFrame();
    }

    for (const auto Type : NativeTypes)
    {
        if (SlowTask.ShouldCancel())
            break;

        Export(Type);
        SlowTask.EnterProgressFrame();
    }

    if (SlowTask.ShouldCancel())
        return;
    ExportUE(NativeTypes);
    ExportUnLua();
    SlowTask.EnterProgressFrame();
}

bool FUnLuaIntelliSenseGenerator::IsBlueprint(const FAssetData& AssetData)
{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
    const FName AssetClass = AssetData.AssetClass;
    return AssetClass == UBlueprint::StaticClass()->GetFName() || AssetClass == UWidgetBlueprint::StaticClass()->GetFName();
#else
    const auto AssetClassPath = AssetData.AssetClassPath.ToString();
    return AssetClassPath == UBlueprint::StaticClass()->GetName() || AssetClassPath == UWidgetBlueprint::StaticClass()->GetName();
#endif
}

bool FUnLuaIntelliSenseGenerator::ShouldExport(const FAssetData& AssetData, bool bLoad)
{
    const auto& Settings = *GetDefault<UUnLuaEditorSettings>();
    if (!Settings.bGenerateIntelliSense)
        return false;

    if (!IsBlueprint(AssetData))
        return false;

    const auto Asset = AssetData.FastGetAsset(bLoad);
    if (!Asset)
        return false;

    const auto Blueprint = Cast<UBlueprint>(Asset);
    if (!Blueprint)
        return false;

    if (Blueprint->SkeletonGeneratedClass || Blueprint->GeneratedClass)
        return true;

    return false;
}

void FUnLuaIntelliSenseGenerator::Export(const UBlueprint* Blueprint)
{
    Export(Blueprint->GeneratedClass);
}

void FUnLuaIntelliSenseGenerator::Export(const UField* Field)
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    const UPackage* Package = Field->GetPackage();
#else
    const UPackage* Package = (UPackage*)Field->GetTypedOuter(UPackage::StaticClass());
#endif
    auto ModuleName = Package->GetName();
    if (!Field->IsNative())
    {
        int32 LastSlashIndex;
        if (ModuleName.FindLastChar('/', LastSlashIndex))
            ModuleName.LeftInline(LastSlashIndex);
    }
    FString FileName = UnLua::IntelliSense::GetTypeName(Field);
    if (FileName.EndsWith("_C"))
        FileName.LeftChopInline(2);
    const FString Content = UnLua::IntelliSense::Get(Field);
    SaveFile(ModuleName, FileName, Content);
}

void FUnLuaIntelliSenseGenerator::ExportUE(const TArray<const UField*> Types)
{
    const FString Content = UnLua::IntelliSense::GetUE(Types);
    SaveFile("", "UE", Content);
}

void FUnLuaIntelliSenseGenerator::ExportUnLua()
{
    const auto ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();
    const auto SrcDir = ContentDir / "IntelliSense";
    const auto DstDir = OutputDir;

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SrcDir))
        return;

    PlatformFile.CopyDirectoryTree(*DstDir, *SrcDir, true);
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
    const FString Directory = OutputDir / ModuleName;
    if (!FileManager.DirectoryExists(*Directory))
        FileManager.MakeDirectory(*Directory);

    const FString FilePath = FString::Printf(TEXT("%s/%s.lua"), *Directory, *FileName);
    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePath);
    if (FileContent != GeneratedFileContent)
        FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void FUnLuaIntelliSenseGenerator::DeleteFile(const FString& ModuleName, const FString& FileName)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString Directory = OutputDir / ModuleName;
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
    if (!ShouldExport(AssetData, true))
        return;

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetData.ObjectPath.ToString());
    if (!Blueprint)
        return;

    Export(Blueprint);
}
