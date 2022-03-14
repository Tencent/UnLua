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
#include "ObjectEditorUtils.h"
#include "ToolMenus.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "BlueprintIntelliSenseGenerator"

static const FName NAME_ToolTip(TEXT("ToolTip"));           // key of ToolTip meta data
static const FName NAME_LatentInfo = TEXT("LatentInfo");    // tag of latent function

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

	//Register BP add/remove/update callbacks
	BPVariablesMap.Reset();
	BPFunctionMap.Reset();
}

void FBlueprintIntelliSenseGenerator::ExportBPFunctions(const UBlueprint* Blueprint)
{
	FString BPName = Blueprint->GetName();
	TArray<FString>& FunctionArr = BPFunctionMap.FindOrAdd(BPName);
	
	// Cache potentially overridable functions
	UClass* ParentClass = *Blueprint->ParentClass;
	for (TFieldIterator<UFunction> FunctionIt(ParentClass, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
	{
		const UFunction* Function = *FunctionIt;
		if (UEdGraphSchema_K2::CanKismetOverrideFunction(Function)
			&& !FObjectEditorUtils::IsFunctionHiddenFromClass(Function, ParentClass)
			&& !FBlueprintEditorUtils::FindOverrideForFunction(Blueprint, CastChecked<UClass>(Function->GetOuter()), Function->GetFName()))
		{
			FunctionArr.Add(GetFunctionStr(Function, BPName));
		}
	}

	// Also functions implemented from interfaces
	for (int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); i++)
	{
		const FBPInterfaceDescription& InterfaceDesc = Blueprint->ImplementedInterfaces[i];
		const UClass* InterfaceClass = InterfaceDesc.Interface.Get();
		if (InterfaceClass)
		{
			for (TFieldIterator<UFunction> FunctionIt(InterfaceClass, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
			{
				const UFunction* Function = *FunctionIt;
				FunctionArr.Add(GetFunctionStr(Function, BPName));
			}
		}
	}

	// Grab functions implemented by the blueprint
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		check(Graph);
		if (Blueprint->SkeletonGeneratedClass != nullptr)
		{
			const UFunction* Function = Blueprint->SkeletonGeneratedClass->FindFunctionByName(Graph->GetFName());
			if (Function != nullptr)
			{
				FunctionArr.Add(GetFunctionStr(Function, BPName));
			}
		}
	}
}

void FBlueprintIntelliSenseGenerator::ExportBPSyntax(const UBlueprint* Blueprint)
{
	FString BPName = Blueprint->GetName();
	TArray<FString>& VarArr = BPVariablesMap.FindOrAdd(BPName);
	VarArr.Reset();

	ExportBPFunctions(Blueprint);

	//export variables
	for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		//UE_LOG(LogTemp, Warning, TEXT("variable name is %s"), *VarDesc.VarName.ToString());
		VarArr.Add(VarDesc.VarName.ToString() + FString(" ") + VarDesc.VarType.PinCategory.ToString());
	}

	//export extra widget variables for UWidgetBlueprint
	const UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (WidgetBlueprint)
	{
		ExportWidgetBPSyntax(WidgetBlueprint);
	}
	ExportSyntaxToFile(BPName);
}

void FBlueprintIntelliSenseGenerator::ExportWidgetBPSyntax(const UWidgetBlueprint* WidgetBlueprint)
{
	//export widget variables
	FString BPName = WidgetBlueprint->GetName();
	TArray<FString>& VarArr = BPVariablesMap.FindOrAdd(BPName);
	WidgetBlueprint->WidgetTree->ForEachWidget([this, &VarArr](const UWidget* Widget)
		{
			if (Widget->bIsVariable)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Widget name is %s %s"), *Widget->GetClass()->GetName(), *Widget->GetName());
				VarArr.Add(*Widget->GetName() + FString(" U") + Widget->GetClass()->GetName());
			}
		});
}

void FBlueprintIntelliSenseGenerator::ExportSyntaxToFile(const FString& BPName)
{
	FString content = "";
	content += "---@class " + BPName + "\r\n";
	TArray<FString>& VarArr = BPVariablesMap.FindOrAdd(BPName);
	TArray<FString>& FuncArr = BPFunctionMap.FindOrAdd(BPName);

	for (FString Var : VarArr)
	{
		content += "---@field public " + Var + "\r\n";
	}
	content += "local M = {}\r\n\r\n";

	for (FString Func : FuncArr)
	{
		content += Func + "\r\n";
	}

	content += "return M";

	SaveFile(FString("Blueprints"), BPName, content);
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
	//filter all UBlueprint
	if (AssetData.AssetClass == UBlueprint::StaticClass()->GetFName() ||
		AssetData.AssetClass == UWidgetBlueprint::StaticClass()->GetFName())
	{
		BPVariablesMap.Remove(AssetData.AssetName.ToString());
		DeleteFile(FString("Blueprints"), AssetData.AssetName.ToString());
	}
}

void FBlueprintIntelliSenseGenerator::OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
	//filter all UBlueprint
	if (AssetData.AssetClass == UBlueprint::StaticClass()->GetFName() ||
		AssetData.AssetClass == UWidgetBlueprint::StaticClass()->GetFName())
	{
		//remove old Blueprint name
		const FString OldPackageName = FPackageName::GetShortName(OldPath);
		BPVariablesMap.Remove(OldPackageName);
		DeleteFile(FString("Blueprints"), OldPackageName);
		
		//update new name 
		OnAssetUpdated(AssetData);
	}
}

void FBlueprintIntelliSenseGenerator::OnAssetUpdated(const FAssetData& AssetData)
{
	//filter all UBlueprint
	if (AssetData.AssetClass == UBlueprint::StaticClass()->GetFName() ||
		AssetData.AssetClass == UWidgetBlueprint::StaticClass()->GetFName())
	{
		//UE_LOG(LogTemp, Warning, TEXT("updated bp name is %s"), *AssetData.GetFullName());
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetData.ObjectPath.ToString());
		if (Blueprint)
		{
			ExportBPSyntax(Blueprint);
		}
	}
}

FString FBlueprintIntelliSenseGenerator::GetFunctionStr(const UFunction* Function, FString ClassName) const
{
	FString Ret;

	// properties
	FString Properties;
	Ret += GetFunctionProperties(Function, Properties);

	// function definition
	Ret += FString::Printf(TEXT("function M%s%s(%s) end\r\n\r\n"), Function->HasAnyFunctionFlags(FUNC_Static) ? TEXT(".") : TEXT(":"), *Function->GetName(), *Properties);

	return Ret;
}

FString FBlueprintIntelliSenseGenerator::GetFunctionProperties(const UFunction* Function, FString& Properties) const
{
	FString Ret;
	TMap<FName, FString>* MetaMap = UMetaData::GetMapForObject(Function);

	// black list of Lua key words
	static FString LuaKeyWords[] = { TEXT("local"), TEXT("function"), TEXT("end") };
	static const int32 NumLuaKeyWords = sizeof(LuaKeyWords) / sizeof(FString);

	for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		FProperty* Property = *It;
		if (Property->GetFName() == NAME_LatentInfo)
		{
			continue;           // filter out 'LatentInfo' parameter
		}

		FString TypeName = GetTypeName(Property);
		const FString& PropertyComment = Property->GetMetaData(NAME_ToolTip);
		FString ExtraDesc;

		if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			Ret += FString::Printf(TEXT("---@return %s"), *TypeName);  // return parameter
		}
		else
		{
			FString PropertyName = Property->GetName();
			for (int32 KeyWordIdx = 0; KeyWordIdx < NumLuaKeyWords; ++KeyWordIdx)
			{
				if (PropertyName == LuaKeyWords[KeyWordIdx])
				{
					PropertyName += TEXT("__");             // add suffix for Lua key words
					break;
				}
			}

			if (Properties.Len() < 1)
			{
				Properties = PropertyName;
			}
			else
			{
				Properties += FString::Printf(TEXT(", %s"), *PropertyName);
			}

			if (MetaMap)
			{
				FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
				FString* ValuePtr = MetaMap->Find(KeyName);     // find default parameter value
				if (ValuePtr)
				{
					ExtraDesc = TEXT("[opt]");                  // default parameter
				}
				else if (Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
				{
					ExtraDesc = TEXT("[out]");                  // non-const reference
				}
			}
			Ret += FString::Printf(TEXT("---@param %s %s"), *PropertyName, *TypeName);
		}

		if (ExtraDesc.Len() > 0 || PropertyComment.Len() > 0)
		{
			Ret += TEXT(" @");
			if (ExtraDesc.Len() > 0)
			{
				Ret += FString::Printf(TEXT("%s "), *ExtraDesc);
			}
		}

		Ret += TEXT("\r\n");
	}

	return Ret;
}

FString FBlueprintIntelliSenseGenerator::GetTypeName(const FProperty* Property) const
{
	check(Property);

	if (Property)
	{
		if (const FByteProperty* TempByteProperty = CastField<FByteProperty>(Property))
		{
			return TEXT("integer");
		}
		else if (const FInt8Property* TempI8Property = CastField<FInt8Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FInt16Property* TempI16Property = CastField<FInt16Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FIntProperty* TempI32Property = CastField<FIntProperty>(Property))
		{
			return TEXT("integer");
		}
		else if (const FInt64Property* TempI64Property = CastField<FInt64Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FUInt16Property* TempU16Property = CastField<FUInt16Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FUInt32Property* TempU32Property = CastField<FUInt32Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FUInt64Property* TempU64Property = CastField<FUInt64Property>(Property))
		{
			return TEXT("integer");
		}
		else if (const FFloatProperty* TempFloatProperty = CastField<FFloatProperty>(Property))
		{
			return TEXT("number");
		}
		else if (const FDoubleProperty* TempDoubleProperty = CastField<FDoubleProperty>(Property))
		{
			return TEXT("number");
		}
		else if (const FEnumProperty* TempEnumProperty = CastField<FEnumProperty>(Property))
		{
			return ((FEnumProperty*)Property)->GetEnum()->GetName();
		}
		else if (const FBoolProperty* TempBoolProperty = CastField<FBoolProperty>(Property))
		{
			return TEXT("boolean");
		}
		else if (const FClassProperty* TempClassProperty = CastField<FClassProperty>(Property))
		{
			UClass* Class = ((FClassProperty*)Property)->MetaClass;
			return FString::Printf(TEXT("TSubclassOf<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FSoftObjectProperty* TempSoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			if (((FSoftObjectProperty*)Property)->PropertyClass->IsChildOf(UClass::StaticClass()))
			{
				UClass* Class = ((FSoftClassProperty*)Property)->MetaClass;
				return FString::Printf(TEXT("TSoftClassPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
			}
			UClass* Class = ((FSoftObjectProperty*)Property)->PropertyClass;
			return FString::Printf(TEXT("TSoftObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FObjectProperty* TempObjectProperty = CastField<FObjectProperty>(Property))
		{
			UClass* Class = ((FObjectProperty*)Property)->PropertyClass;
			return FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FWeakObjectProperty* TempWeakObjectProperty = CastField<FWeakObjectProperty>(Property))
		{
			UClass* Class = ((FWeakObjectProperty*)Property)->PropertyClass;
			return FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FLazyObjectProperty* TempLazyObjectProperty = CastField<FLazyObjectProperty>(Property))
		{
			UClass* Class = ((FLazyObjectProperty*)Property)->PropertyClass;
			return FString::Printf(TEXT("TLazyObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FInterfaceProperty* TempInterfaceProperty = CastField<FInterfaceProperty>(Property))
		{
			UClass* Class = ((FInterfaceProperty*)Property)->InterfaceClass;
			return FString::Printf(TEXT("TScriptInterface<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
		}
		else if (const FNameProperty* TempNameProperty = CastField<FNameProperty>(Property))
		{
			return TEXT("string");
		}
		else if (const FStrProperty* TempStringProperty = CastField<FStrProperty>(Property))
		{
			return TEXT("string");
		}
		else if (const FTextProperty* TempTextProperty = CastField<FTextProperty>(Property))
		{
			return TEXT("string");
		}
		else if (const FArrayProperty* TempArrayProperty = CastField<FArrayProperty>(Property))
		{
			FProperty* Inner = ((FArrayProperty*)Property)->Inner;
			return FString::Printf(TEXT("TArray<%s>"), *GetTypeName(Inner));
		}
		else if (const FMapProperty* TempMapProperty = CastField<FMapProperty>(Property))
		{
			FProperty* KeyProp = ((FMapProperty*)Property)->KeyProp;
			FProperty* ValueProp = ((FMapProperty*)Property)->ValueProp;
			return FString::Printf(TEXT("TMap<%s, %s>"), *GetTypeName(KeyProp), *GetTypeName(ValueProp));
		}
		else if (const FSetProperty* TempSetProperty = CastField<FSetProperty>(Property))
		{
			FProperty* ElementProp = ((FSetProperty*)Property)->ElementProp;
			return FString::Printf(TEXT("TSet<%s>"), *GetTypeName(ElementProp));
		}
		else if (const FStructProperty* TempStructProperty = CastField<FStructProperty>(Property))
		{
			return ((FStructProperty*)Property)->Struct->GetStructCPPName();
		}
		else if (const FDelegateProperty* TempDelegateProperty = CastField<FDelegateProperty>(Property))
		{
			return TEXT("Delegate");
		}
		else if (const FMulticastDelegateProperty* TempMulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property))
		{
			return TEXT("MulticastDelegate");
		}
	}

	return TEXT("Unknown");
}
