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

class FUnLuaDefaultParamCollectorModule : public IScriptGeneratorPluginInterface
{
public:
    virtual void StartupModule() override { IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual void ShutdownModule() override { IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual FString GetGeneratedCodeModuleName() const override { return TEXT("UnLua"); }
    virtual bool SupportsTarget(const FString& TargetName) const override { return true; }

    virtual bool ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType, const FString& ModuleGeneratedIncludeDirectory) const override
    {
        return ModuleType == EBuildModuleType::EngineRuntime || ModuleType == EBuildModuleType::GameRuntime;    // only 'EngineRuntime' and 'GameRuntime' are valid
    }
    
    virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, const FString& IncludeBase) override
    {
        GeneratedFileContent.Empty();
        GeneratedFileContent += FString::Printf(TEXT("FFunctionCollection *FC = nullptr;\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FParameterCollection *PC = nullptr;\r\n\r\n"));

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
            UFunction *Function = *FuncIt;
            CurrentFunctionName.Empty();

            // filter out functions without meta data
            TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);
            if (!MetaMap)
            {
                continue;
            }

            // parameters
            for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
            {
                FProperty *Property = *It;

                // filter out properties without default value
                FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
                FString *ValuePtr = MetaMap->Find(KeyName);
                if (!ValuePtr)
                {
                    continue;
                }

                const FString &ValueStr = *ValuePtr;
                if (Property->IsA(FStructProperty::StaticClass()))
                {
                    // get all possible script structs
                    UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
                    static const UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector"));
                    static const UScriptStruct* Vector2DStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector2D"));
                    static const UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Rotator"));
                    static const UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("LinearColor"));
                    static const UScriptStruct* ColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Color"));

                    const FStructProperty *StructProperty = CastField<FStructProperty>(Property);
                    if (StructProperty->Struct == VectorStruct)                     // FVector
                    {
                        TArray<FString> Values;
                        ValueStr.ParseIntoArray(Values, TEXT(","));
                        if (Values.Num() == 3)
                        {
                            float X = TCString<TCHAR>::Atof(*Values[0]);
                            float Y = TCString<TCHAR>::Atof(*Values[1]);
                            float Z = TCString<TCHAR>::Atof(*Values[2]);
                            if (FVector::ZeroVector.Equals(FVector(X, Y, Z)))
                            {
                                continue;
                            }
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVectorParamValue(FVector(%ff,%ff,%ff)));\r\n"), *Property->GetName(), X, Y, Z);
                        }
                    }
                    else if (StructProperty->Struct == RotatorStruct)               // FRotator
                    {
                        TArray<FString> Values;
                        ValueStr.ParseIntoArray(Values, TEXT(","));
                        if (Values.Num() == 3)
                        {
                            float Pitch = TCString<TCHAR>::Atof(*Values[0]);
                            float Yaw = TCString<TCHAR>::Atof(*Values[1]);
                            float Roll = TCString<TCHAR>::Atof(*Values[2]);
                            if (FRotator::ZeroRotator.Equals(FRotator(Pitch, Yaw, Roll)))
                            {
                                continue;
                            }
                            PreAddProperty(Class, Function);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRotatorParamValue(FRotator(%ff,%ff,%ff)));\r\n"), *Property->GetName(), Pitch, Yaw, Roll);
                        }
                    }
                    else if (StructProperty->Struct == Vector2DStruct)              // FVector2D
                    {
                        FVector2D Value;
                        Value.InitFromString(ValueStr);
                        if (FVector2D::ZeroVector.Equals(Value))
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVector2DParamValue(FVector2D(%ff,%ff)));\r\n"), *Property->GetName(), Value.X, Value.Y);
                    }
                    else if (StructProperty->Struct == LinearColorStruct)           // FLinearColor
                    {
                        static FLinearColor ZeroLinearColor(ForceInit);
                        FLinearColor Value;
                        Value.InitFromString(ValueStr);
                        if (ZeroLinearColor.Equals(Value))
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FLinearColorParamValue(FLinearColor(%ff,%ff,%ff,%ff)));\r\n"), *Property->GetName(), Value.R, Value.G, Value.B, Value.A);
                    }
                    else if (StructProperty->Struct == ColorStruct)                 // FColor
                    {
                        static FColor ZeroColor(ForceInit);
                        FColor Value;
                        Value.InitFromString(ValueStr);
                        if (ZeroColor == Value)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FColorParamValue(FColor(%d,%d,%d,%d)));\r\n"), *Property->GetName(), Value.R, Value.G, Value.B, Value.A);
                    }
                }
                else
                {
                    if (Property->IsA(FIntProperty::StaticClass()))                 // int
                    {
                        int32 Value = TCString<TCHAR>::Atoi(*ValueStr);
                        if (Value == 0)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FIntParamValue(%d));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FByteProperty::StaticClass()))           // byte
                    {
                        const UEnum *Enum = CastField<FByteProperty>(Property)->Enum;
                        int32 Value = Enum ? (int32)Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi(*ValueStr);
                        check(Value >= 0 && Value <= 255);
                        if (Value == 0)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FByteParamValue(%d));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FEnumProperty::StaticClass()))           // enum
                    {
                        const UEnum *Enum = CastField<FEnumProperty>(Property)->GetEnum();
                        int64 Value = Enum ? Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi64(*ValueStr);
                        if (Value == 0)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FEnumParamValue(%ld));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FFloatProperty::StaticClass()))          // float
                    {
                        float Value = TCString<TCHAR>::Atof(*ValueStr);
                        if (FMath::IsNearlyZero(Value))
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FFloatParamValue(%ff));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FDoubleProperty::StaticClass()))         // double
                    {
                        double Value = TCString<TCHAR>::Atod(*ValueStr);
                        if (FMath::IsNearlyZero(Value))
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FDoubleParamValue(%lf));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(FBoolProperty::StaticClass()))           // boolean
                    {
                        static FString FalseValue(TEXT("false"));
                        if (ValueStr == FalseValue)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FBoolParamValue(true));\r\n"), *Property->GetName());
                    }
                    else if (Property->IsA(FNameProperty::StaticClass()))           // FName
                    {
                        static FString NoneValue(TEXT("None"));
                        if (ValueStr == NoneValue)
                        {
                            continue;
                        }
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FNameParamValue(FName(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                    else if (Property->IsA(FTextProperty::StaticClass()))           // FText
                    {
                        PreAddProperty(Class, Function);
#if ENGINE_MINOR_VERSION > 20
                        if (ValueStr.StartsWith(TEXT("INVTEXT(\"")))
                        {
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(%s));\r\n"), *Property->GetName(), *ValueStr);
                        }
                        else
#endif
                        {
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(FText::FromString(TEXT(\"%s\"))));\r\n"), *Property->GetName(), *ValueStr);
                        }
                    }
                    else if (Property->IsA(FStrProperty::StaticClass()))            // FString
                    {
                        PreAddProperty(Class, Function);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FStringParamValue(TEXT(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                    else
                    {
                        continue;
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
        if (FileContent != GeneratedFileContent)
        {
            bool bResult = FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
            check(bResult);
        }
    }

    virtual FString GetGeneratorName() const override
    {
        return TEXT("UnLua Default Parameters Collector");
    }

private:
    void PreAddProperty(UClass *Class, UFunction *Function)
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

    FString OutputDir;
    FString CurrentClassName;
    FString CurrentFunctionName;
    FString GeneratedFileContent;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaDefaultParamCollectorModule, UnLuaDefaultParamCollector)
