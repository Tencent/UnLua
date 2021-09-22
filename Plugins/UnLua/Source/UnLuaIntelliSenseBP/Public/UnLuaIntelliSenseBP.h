#pragma once

#include "CoreUObject.h"

#include "Modules/ModuleManager.h"

class UBlueprint;
class UWidgetBlueprint;

class FUnLuaIntelliSenseBPModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
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

	//Update all Blueprints from command
	void UpdateAll();
	void RegisterAssetChange();

private:
	// Get function IntelliSense
	FString GetFunctionStr(const UFunction* Function, FString ClassName) const;

	// Get function properties string
	FString GetFunctionProperties(const UFunction* Function, FString& Properties) const;

	// Get readable type name for a UPROPERTY
	FString GetTypeName(const UProperty* Property) const;
private:

	TSharedPtr<class FUICommandList> PluginCommands;

	/** store BP variable information in map, key is blueprint asset name */
	TMap<FString,TArray<FString>> BPVariablesMap;

	/** store BP function information in map, key is blueprint asset name */
	TMap<FString, TArray<FString>> BPFunctionMap;

	/** The folder of .lua to export. */
	FString OutputDir;

	/** The content of catalog */
	FString CatelogContent;

	/** Register AssetChange flag. */
	bool AssetChangeRegistered;

};
