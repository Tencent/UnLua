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

#include "CoreUObject.h"
#include "Features/IModularFeatures.h"
#include "IScriptGeneratorPluginInterface.h"
#include "UnLuaCompatibility.h"

#define LOCTEXT_NAMESPACE "FUnLuaDefaultParamCollectorModule"

static bool FindDefaultValueString(const TMap<FName, FString>* MetaMap, const FProperty* Param, FString& OutString)
{
    check(MetaMap && Param);

    const FName ParamName(*Param->GetName());
    const FString* DefaultValue = MetaMap->Find(ParamName);
    if (DefaultValue)
    {
        // Specified default value in the UFUNCTION metadata
        OutString = *DefaultValue;
        return true;
    }

    const FName CppKey(*(FString(TEXT("CPP_Default_")) + Param->GetName()));
    const FString* CppDefaultValue = MetaMap->Find(CppKey);
    if (CppDefaultValue)
    {
        // Default value in the function signature
        OutString = *CppDefaultValue;
        return true;
    }

    return false;
}

class FUnLuaDefaultParamCollectorModule : public IScriptGeneratorPluginInterface
{
public:
    virtual void StartupModule() override
    {
        IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this);
        HasGameRuntime = false;
    }

    virtual void ShutdownModule() override
    {
        IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this);
    }

    virtual FString GetGeneratedCodeModuleName() const override { return TEXT("UnLua"); }

    virtual bool SupportsTarget(const FString& TargetName) const override { return true; }

    virtual bool ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType, const FString& ModuleGeneratedIncludeDirectory) const override
    {
        FUnLuaDefaultParamCollectorModule* NonConstPtr = const_cast<FUnLuaDefaultParamCollectorModule*>(this);
        NonConstPtr->ParseModule(ModuleName, ModuleType, ModuleGeneratedIncludeDirectory);
        return ModuleType == EBuildModuleType::EngineRuntime || ModuleType == EBuildModuleType::GameRuntime; // only 'EngineRuntime' and 'GameRuntime' are valid
    }

    virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, const FString& IncludeBase) override
    {
        GeneratedFileContent.Empty();

        GeneratedFileContent += FString::Printf(TEXT("FFunctionCollection* FC = nullptr;\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FParameterCollection* PC = nullptr;\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FBoolParamValue* SharedBool_TRUE = new FBoolParamValue(true);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FBoolParamValue* SharedBool_FALSE = new FBoolParamValue(false);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FFloatParamValue* SharedFloat_Zero = new FFloatParamValue(0.000000f);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FFloatParamValue* SharedFloat_One = new FFloatParamValue(1.000000f);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FEnumParamValue* SharedEnum_Zero = new FEnumParamValue(0);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FIntParamValue* SharedInt_Zero = new FIntParamValue(0);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FByteParamValue* SharedByte_Zero = new FByteParamValue(0);\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FNameParamValue* SharedFName_None = new FNameParamValue(FName(\"None\"));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FVectorParamValue* SharedFVector_Zero = new FVectorParamValue(FVector(EForceInit::ForceInitToZero));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FVector2DParamValue* SharedFVector2D_Zero = new FVector2DParamValue(FVector2D(EForceInit::ForceInitToZero));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FRotatorParamValue* SharedFRotator_Zero = new FRotatorParamValue(FRotator(EForceInit::ForceInitToZero));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FLinearColorParamValue* SharedFLinearColor_Zero = new FLinearColorParamValue(FLinearColor(EForceInit::ForceInitToZero));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FColorParamValue* SharedFColor_Zero = new FColorParamValue(FColor(EForceInit::ForceInitToZero));\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FScriptArrayParamValue* SharedScriptArray = new FScriptArrayParamValue();\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FScriptDelegateParamValue* SharedScriptDelegate = new FScriptDelegateParamValue(FScriptDelegate());\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FMulticastScriptDelegateParamValue* SharedMulticastScriptDelegate = new FMulticastScriptDelegateParamValue(FMulticastScriptDelegate());\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("\r\n"));

        OutputDir = OutputDirectory;
    }

    virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) override
    {
        // #lizard forgives

        // filter out interfaces
        if (Class->HasAnyClassFlags(CLASS_Interface))
        {
            return;
        }

        for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); FuncIt; ++FuncIt)
        {
            UFunction* Function = *FuncIt;
            CurrentFunctionName.Empty();

            // filter out functions without meta data
            TMap<FName, FString>* MetaMap = UMetaData::GetMapForObject(Function);
            if (!MetaMap)
            {
                continue;
            }

            const FString& AutoCreateRefTerm = MetaMap->FindRef("AutoCreateRefTerm");
            TArray<FString> AutoEmitParameterNames;
            if (!AutoCreateRefTerm.IsEmpty())
            {
                AutoCreateRefTerm.ParseIntoArray(AutoEmitParameterNames, TEXT(","), true);
                for (FString& ParamName : AutoEmitParameterNames)
                    ParamName.TrimStartAndEndInline();
                // GeneratedFileContent += FString::Printf(TEXT("// DEBUG %s AutoCreateRefTerm=%s \r\n"), *Function->GetName(), *AutoCreateRefTerm);
            }

            // parameters
            for (TFieldIterator<FProperty> It(Function); It && (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm)); ++It)
            {
                FProperty* Property = *It;
                FString ValueStr;

                // filter out properties without default value
                if (!FindDefaultValueString(MetaMap, Property, ValueStr))
                {
                    if (AutoEmitParameterNames.Find(Property->GetName()) == INDEX_NONE)
                    {
                        // GeneratedFileContent += FString::Printf(TEXT("// DEBUG %s has no default value\r\n"),  *Property->GetName());
                        continue;
                    }
                    // GeneratedFileContent += FString::Printf(TEXT("// DEBUG %s AutoEmitParameterNames(%d) %s\r\n"),
                    //                                         *Property->GetName(), AutoEmitParameterNames.Find(Property->GetName()), *AutoCreateRefTerm);
                }

                if (Property->IsA(FStructProperty::StaticClass()))
                {
                    // get all possible script structs
                    UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
                    static const UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector"));
                    static const UScriptStruct* Vector2DStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector2D"));
                    static const UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Rotator"));
                    static const UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("LinearColor"));
                    static const UScriptStruct* ColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Color"));

                    const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
                    if (StructProperty->Struct == VectorStruct) // FVector
                    {
                        if(ValueStr.IsEmpty())
                        {
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFVector_Zero);\r\n"), *Property->GetName());
                        }
                        else
                        {
                            TArray<FString> Values;
                            ValueStr.ParseIntoArray(Values, TEXT(","));
                            if (Values.Num() == 3)
                            {
                                float X = TCString<TCHAR>::Atof(*Values[0]);
                                float Y = TCString<TCHAR>::Atof(*Values[1]);
                                float Z = TCString<TCHAR>::Atof(*Values[2]);
                                PreAddProperty(Class, Function);
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVectorParamValue(FVector(%ff,%ff,%ff)));\r\n"), *Property->GetName(), X, Y, Z);
                            }
                        }
                    }
                    else if (StructProperty->Struct == RotatorStruct) // FRotator
                    {
                        if(ValueStr.IsEmpty())
                        {
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFRotator_Zero);\r\n"), *Property->GetName());
                        }
                        else
                        {
                            TArray<FString> Values;
                            ValueStr.ParseIntoArray(Values, TEXT(","));
                            if (Values.Num() == 3)
                            {
                                float Pitch = TCString<TCHAR>::Atof(*Values[0]);
                                float Yaw = TCString<TCHAR>::Atof(*Values[1]);
                                float Roll = TCString<TCHAR>::Atof(*Values[2]);
                                PreAddProperty(Class, Function);
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRotatorParamValue(FRotator(%ff,%ff,%ff)));\r\n"),
                                                                        *Property->GetName(), Pitch, Yaw, Roll);
                            }
                        }
                    }
                    else if (StructProperty->Struct == Vector2DStruct) // FVector2D
                    {
                        if(ValueStr.IsEmpty())
                        {
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFVector2D_Zero);\r\n"), *Property->GetName());
                        }
                        else
                        {
                            FVector2D Value;
                            Value.InitFromString(ValueStr);
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVector2DParamValue(FVector2D(%ff,%ff)));\r\n"), *Property->GetName(), Value.X, Value.Y);
                        }
                    }
                    else if (StructProperty->Struct == LinearColorStruct) // FLinearColor
                    {
                        if(ValueStr.IsEmpty())
                        {
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFLinearColor_Zero);\r\n"), *Property->GetName());
                        }
                        else
                        {
                            FLinearColor Value;
                            Value.InitFromString(ValueStr);
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FLinearColorParamValue(FLinearColor(%ff,%ff,%ff,%ff)));\r\n"),
                                                                    *Property->GetName(), Value.R, Value.G, Value.B, Value.A);    
                        }
                    }
                    else if (StructProperty->Struct == ColorStruct) // FColor
                    {
                        if(ValueStr.IsEmpty())
                        {
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFColor_Zero);\r\n"), *Property->GetName());
                        }
                        else
                        {
                            FColor Value;
                            Value.InitFromString(ValueStr);
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FColorParamValue(FColor(%d,%d,%d,%d)));\r\n"),
                                                                    *Property->GetName(), Value.R, Value.G, Value.B, Value.A);   
                        }
                    }
                }
                else
                {
                    if (Property->IsA(FIntProperty::StaticClass())) // int
                    {
                        int32 Value = TCString<TCHAR>::Atoi(*ValueStr);
                        PreAddProperty(Class, Function);
                        if (Value == 0)
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedInt_Zero);\r\n"), *Property->GetName(), Value);
                        else
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FIntParamValue(%d));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FByteProperty::StaticClass())) // byte
                    {
                        const UEnum* Enum = CastField<FByteProperty>(Property)->Enum;
                        if(Enum)
                        {
                            int64 Value = Enum->GetValueByNameString(ValueStr);
                            PreAddProperty(Class, Function);
                            if (Value == 0)
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedByte_Zero);\r\n"), *Property->GetName(), Value);
                            }
                            else if (Value > 0 && Value <= 255)
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FByteParamValue(%d));\r\n"), *Property->GetName(), Value);
                            }
                            else
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRuntimeEnumParamValue(\"%s\",%d));\r\n"), *Property->GetName(), *Enum->CppType, Enum->GetIndexByNameString(ValueStr));
                            }
                        }
                        else
                        {
                            int32 Value = TCString<TCHAR>::Atoi(*ValueStr);
                            check(Value >= 0 && Value <= 255)
                            PreAddProperty(Class, Function);
                            if (Value == 0)
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedByte_Zero);\r\n"), *Property->GetName(), Value);
                            else
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FByteParamValue(%d));\r\n"), *Property->GetName(), Value);
                        }
                    }
                    else if (Property->IsA(FEnumProperty::StaticClass())) // enum
                    {
                        const UEnum* Enum = CastField<FEnumProperty>(Property)->GetEnum();
                        if(Enum)
                        {
                            int64 Value = Enum->GetValueByNameString(ValueStr);
                            PreAddProperty(Class, Function);
                            if (Value == 0)
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedEnum_Zero);\r\n"), *Property->GetName(), Value);
                            }
                            else if (Value > 0 && Value <= 255)
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FEnumParamValue(%ld));\r\n"), *Property->GetName(), Value);
                            }
                            else
                            {
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRuntimeEnumParamValue(\"%s\",%d));\r\n"), *Property->GetName(), *Enum->CppType, Enum->GetIndexByNameString(ValueStr));
                            }
                        }
                        else
                        {
                            int64 Value = TCString<TCHAR>::Atoi64(*ValueStr);
                            PreAddProperty(Class, Function);
                            if (Value == 0)
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedEnum_Zero);\r\n"), *Property->GetName(), Value);
                            else
                                GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FEnumParamValue(%ld));\r\n"), *Property->GetName(), Value);
                        }
                    }
                    else if (Property->IsA(FFloatProperty::StaticClass())) // float
                    {
                        float Value = TCString<TCHAR>::Atof(*ValueStr);
                        PreAddProperty(Class, Function);
                        if (FMath::IsNearlyZero(Value))
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFloat_Zero);\r\n"), *Property->GetName(), Value);
                        else if (FMath::IsNearlyEqual(Value, 1))
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFloat_One);\r\n"), *Property->GetName(), Value);
                        else
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FFloatParamValue(%ff));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FDoubleProperty::StaticClass())) // double
                    {
                        double Value = TCString<TCHAR>::Atod(*ValueStr);
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FDoubleParamValue(%lf));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FBoolProperty::StaticClass())) // boolean
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedBool_%s);\r\n"), *Property->GetName(), *ValueStr.ToUpper());
                    }
                    else if (Property->IsA(FNameProperty::StaticClass())) // FName
                    {
                        PreAddProperty(Class, Function);
                        if (ValueStr == "None")
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedFName_None);\r\n"), *Property->GetName(), *ValueStr);
                        else
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FNameParamValue(FName(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                    else if (Property->IsA(FTextProperty::StaticClass())) // FText
                    {
                        PreAddProperty(Class, Function);
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 20)
                        if (ValueStr.StartsWith(TEXT("INVTEXT(\"")))
                        {
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(%s));\r\n"), *Property->GetName(), *ValueStr);
                        }
                        else
#endif
                        {
                            GeneratedFileContent += FString::Printf(
                                TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(FText::FromString(TEXT(\"%s\"))));\r\n"), *Property->GetName(), *ValueStr);
                        }
                    }
                    else if (Property->IsA(FStrProperty::StaticClass())) // FString
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FStringParamValue(TEXT(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                    else if (Property->IsA(FArrayProperty::StaticClass()))
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedScriptArray);\r\n"), *Property->GetName());
                    }
                    else if (Property->IsA(FDelegateProperty::StaticClass()))
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedScriptDelegate);\r\n"), *Property->GetName());
                    }
                    else if (Property->IsA(FMulticastDelegateProperty::StaticClass()))
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), SharedMulticastDelegate);\r\n"), *Property->GetName());
                    }
                }
            }
        }

        if (CurrentClassName.Len() > 0)
        {
            CurrentClassName.Empty();
            CurrentFunctionName.Empty();

            GeneratedFileContent += TEXT("\r\n");
        }
    }

    virtual void FinishExport() override
    {
        const FString FilePath = FString::Printf(TEXT("%s%s"), *OutputDir, TEXT("DefaultParamCollection.inl"));
        FString FileContent;
        FFileHelper::LoadFileToString(FileContent, *FilePath);

        // If Current build Game Project, try to update DefaultParamCollection.inl file for project
        if (HasGameRuntime)
        {
            if (GeneratedFileContent != FileContent)
            {
                bool bResult = FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
                check(bResult);
            }
        }
        else
        {
            // If Current build Engine Project, try create new file if has no DefaultParamCollection.inl to fix compile error
            // or do not update DefaultParamCollection.inl file if exists
            if (!FPaths::FileExists(FilePath) || FileContent.Len() == 0)
            {
                bool bResult = FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
                check(bResult);
            }
        }
    }

    virtual FString GetGeneratorName() const override
    {
        return TEXT("UnLua Default Parameters Collector");
    }

private:
    void PreAddProperty(UClass* Class, UFunction* Function)
    {
        if (CurrentClassName.Len() < 1)
        {
            CurrentClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
            GeneratedFileContent += FString::Printf(TEXT("FC = &GDefaultParamCollection.Add(TEXT(\"%s\"));\r\n"), *CurrentClassName);
        }
        if (CurrentFunctionName.Len() < 1)
        {
            CurrentFunctionName = Function->GetName();
            GeneratedFileContent += FString::Printf(TEXT("PC = &FC->Functions.Add(TEXT(\"%s\"));\r\n"), *CurrentFunctionName);
        }
    }

    void ParseModule(const FString& ModuleName, EBuildModuleType::Type ModuleType, const FString& ModuleGeneratedIncludeDirectory)
    {
        GeneratedFileContent += FString::Printf(TEXT("// ModuleName %s Type %d  ModuleGeneratedIncludeDirectory %s \r\n"), *ModuleName, ModuleType, *ModuleGeneratedIncludeDirectory);
        if (ModuleType == EBuildModuleType::GameRuntime)
        {
            // For Only Game Project has GameRuntime Module, this should be Game Project
            HasGameRuntime = true;
        }
    }

    bool HasGameRuntime; // Flag for if current uht has GameRuntime module or not
    FString OutputDir;
    FString CurrentClassName;
    FString CurrentFunctionName;
    FString GeneratedFileContent;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaDefaultParamCollectorModule, UnLuaDefaultParamCollector)
