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

#define LOCTEXT_NAMESPACE "FUnLuaIntelliSenseModule"

#ifndef ENABLE_INTELLISENSE
#define ENABLE_INTELLISENSE 0
#endif

static const FName NAME_ToolTip(TEXT("ToolTip"));           // key of ToolTip meta data
static const FName NAME_LatentInfo = TEXT("LatentInfo");    // tag of latent function

class FUnLuaIntelliSenseModule : public IScriptGeneratorPluginInterface
{
public:
    virtual void StartupModule() override { IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual void ShutdownModule() override { IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual FString GetGeneratedCodeModuleName() const override { return TEXT("UnLua"); }
    virtual bool SupportsTarget(const FString& TargetName) const override { return ENABLE_INTELLISENSE ? true : false; }

    virtual bool ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType, const FString& ModuleGeneratedIncludeDirectory) const override
    {
        return ModuleType == EBuildModuleType::EngineRuntime || ModuleType == EBuildModuleType::GameRuntime;    // only 'EngineRuntime' and 'GameRuntime' are valid
    }
    
    virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, const FString& IncludeBase) override
    {
        OutputDir = IncludeBase + TEXT("../Intermediate/");     // export symbol (IntelliSense) files to plugin's 'Intermediate' dir
        UE4Namespace = TEXT("_G.UE4 = {\r\n\r\n");
    }

    virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) override
    {
        // filter out 'EditorOnly' and 'Developer' packages
        UPackage *Package = Cast<UPackage>(Class->GetOuter());
        check(Package);
        if (Package->HasAnyPackageFlags(PKG_EditorOnly | PKG_Developer))
        {
            return;
        }

        // filter out deprecated class
        if (Class->HasAnyClassFlags(CLASS_Deprecated))
        {
            return;
        }

        FString ClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
        int32 Num = UE4Names.Num();
        int32 Idx = UE4Names.AddUnique(ClassName);
        if (Idx < Num)
        {
            return;
        }

        FString ModuleName = GetModuleName(Package);

        FString GeneratedFileContent;
        ExportUStructDefinition(Class, GeneratedFileContent);       // struct definition

        // functions
        for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); FuncIt; ++FuncIt)
        {
            UFunction *Function = *FuncIt;

            // function comment
            ExportMultiLineComments(Function->GetMetaData(NAME_ToolTip), GeneratedFileContent);

            // properties
            FString Properties;
            ExportProperties(Function, Properties, GeneratedFileContent);
            
            // function definition
            GeneratedFileContent += FString::Printf(TEXT("function %s%s%s(%s) end\r\n\r\n"), *ClassName, Function->HasAnyFunctionFlags(FUNC_Static) ? TEXT(".") : TEXT(":"), *Function->GetName(), *Properties);
        }

        // return
        GeneratedFileContent += FString::Printf(TEXT("return %s\r\n"), *ClassName);

        // save to lua file
        SaveFile(ModuleName, ClassName, GeneratedFileContent);
    }

    virtual void FinishExport() override
    {
        // structs
        for (TObjectIterator<UScriptStruct> It; It; ++It)
        {
            UScriptStruct *Struct = *It;

            // filter out 'EditorOnly' and 'Developer' packages
            UPackage *Package = Cast<UPackage>(Struct->GetOuter());
            check(Package);
            if (Package->HasAnyPackageFlags(PKG_EditorOnly | PKG_Developer))
            {
                continue;
            }

            FString StructName = FString::Printf(TEXT("%s%s"), Struct->GetPrefixCPP(), *Struct->GetName());
            UE4Names.Add(StructName);

            FString ModuleName = GetModuleName(Package);

            // struct definition
            FString GeneratedFileContent;
            ExportUStructDefinition(Struct, GeneratedFileContent);

            // return
            GeneratedFileContent += FString::Printf(TEXT("return %s\r\n"), *StructName);

            // save to lua file
            SaveFile(ModuleName, StructName, GeneratedFileContent);
        }

        // enums
        for (TObjectIterator<UEnum> It; It; ++It)
        {
            UEnum *Enum = *It;

            // filter out 'EditorOnly' and 'Developer' packages
            UPackage *Package = Cast<UPackage>(Enum->GetOuter());
            check(Package);
            if (Package->HasAnyPackageFlags(PKG_EditorOnly | PKG_Developer))
            {
                continue;
            }

            FString EnumName = Enum->GetName();
            UE4Names.Add(EnumName);

            FString ModuleName = GetModuleName(Package);
            FString GeneratedFileContent;

            // comment
            ExportMultiLineComments(Enum->GetMetaData(TEXT("ToolTip")), GeneratedFileContent);

            // enum name
            GeneratedFileContent += FString::Printf(TEXT("---@class %s"), *EnumName);

            // entries
            int32 NumEnums = Enum->NumEnums();
            for (int32 i = 0; i < NumEnums; ++i)
            {
                GeneratedFileContent += FString::Printf(TEXT("\r\n---@field %s %s %s"), TEXT("public"), *Enum->GetNameStringByIndex(i), TEXT("integer"));
            }

            // definition
            GeneratedFileContent += FString::Printf(TEXT("\r\nlocal %s = {}\r\n\r\n"), *EnumName);

            // return
            GeneratedFileContent += FString::Printf(TEXT("return %s\r\n"), *EnumName);

            // save to lua file
            SaveFile(ModuleName, EnumName, GeneratedFileContent);
        }

        // save 'UE4Namespace' to lua file
        UE4Names.Sort();
        for (const FString &Name : UE4Names)
        {
            UE4Namespace += FString::Printf(TEXT("    ---@type %s\r\n"), *Name);
            UE4Namespace += FString::Printf(TEXT("    %s = nil,\r\n\r\n"), *Name);
        }
        UE4Namespace += TEXT("    _UE4_END_ = nil\r\n\r\n}\r\n");
        SaveFile(TEXT(""), TEXT("UE4"), UE4Namespace);
    }

    virtual FString GetGeneratorName() const override
    {
        return TEXT("UnLua IntelliSense");
    }

private:
    void ExportUStructDefinition(UStruct *Struct, FString &GeneratedFileContent)
    {
        FString StructName = FString::Printf(TEXT("%s%s"), Struct->GetPrefixCPP(), *Struct->GetName());

        // comment
        ExportMultiLineComments(Struct->GetMetaData(NAME_ToolTip), GeneratedFileContent);

        // struct name
        GeneratedFileContent += FString::Printf(TEXT("---@class %s"), *StructName);
        UStruct *ParentStruct = Struct->GetSuperStruct();
        if (ParentStruct)
        {
            GeneratedFileContent += FString::Printf(TEXT(" : %s%s"), ParentStruct->GetPrefixCPP(), *ParentStruct->GetName());
        }

        // fields
        for (TFieldIterator<FProperty> PropertyIt(Struct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); PropertyIt; ++PropertyIt)
        {
            FProperty *Property = *PropertyIt;

            // access level
            FString AccessLevel;
            if (Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
            {
                AccessLevel = TEXT("public");
            }
            else if (Property->HasAllPropertyFlags(CPF_NativeAccessSpecifierProtected))
            {
                AccessLevel = TEXT("protected");
            }
            else
            {
                AccessLevel = TEXT("private");
            }

            FString TypeName = GetTypeName(Property);
            GeneratedFileContent += FString::Printf(TEXT("\r\n---@field %s %s %s"), *AccessLevel, *Property->GetName(), *TypeName);

            // comment
            const FString &PropertyComment = Property->GetMetaData(NAME_ToolTip);
            if (PropertyComment.Len() > 0)
            {
                ExportPropertyComment(PropertyComment, GeneratedFileContent);
                //GeneratedFileContent += FString::Printf(TEXT(" @%s"), *PropertyComment.Replace(TEXT("\n"), TEXT(", ")));
            }
        }

        // definition
        GeneratedFileContent += FString::Printf(TEXT("\r\nlocal %s = {}\r\n\r\n"), *StructName);
    }

    void ExportProperties(UFunction *Function, FString &Properties, FString &GeneratedFileContent)
    {
        TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);

        // black list of Lua key words
        static FString LuaKeyWords[] = { TEXT("local"), TEXT("function"), TEXT("end") };
        static const int32 NumLuaKeyWords = sizeof(LuaKeyWords) / sizeof(FString);

        for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
        {
            FProperty *Property = *It;
            if (Property->GetFName() == NAME_LatentInfo)
            {
                continue;           // filter out 'LatentInfo' parameter
            }

            FString TypeName = GetTypeName(Property);
            const FString &PropertyComment = Property->GetMetaData(NAME_ToolTip);
            FString ExtraDesc;

            if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                GeneratedFileContent += FString::Printf(TEXT("---@return %s"), *TypeName);  // return parameter
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

                FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
                FString *ValuePtr = MetaMap->Find(KeyName);     // find default parameter value
                if (ValuePtr)
                {
                    ExtraDesc = TEXT("[opt]");                  // default parameter
                }
                else if (Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
                {
                    ExtraDesc = TEXT("[out]");                  // non-const reference
                }

                GeneratedFileContent += FString::Printf(TEXT("---@param %s %s"), *PropertyName, *TypeName);
            }

            if (ExtraDesc.Len() > 0 || PropertyComment.Len() > 0)
            {
                GeneratedFileContent += TEXT(" @");
                if (ExtraDesc.Len() > 0)
                {
                    GeneratedFileContent += FString::Printf(TEXT("%s "), *ExtraDesc);
                }
                if (PropertyComment.Len() > 0)
                {
                    ExportPropertyComment(PropertyComment, GeneratedFileContent);
                    //GeneratedFileContent += FString::Printf(TEXT(" @%s"), *PropertyComment.Replace(TEXT("\n"), TEXT(", ")));
                }
            }

            GeneratedFileContent += TEXT("\r\n");
        }
    }

    // parse a single line comment
    FString GetValidComment(const FString &Line)
    {
        int32 Index = 0;
        for (; Index < Line.Len(); ++Index)
        {
            if (Line[Index] != ' ')
            {
                break;
            }
        }
        if (Line[Index] == '@')
        {
            return TEXT("");
        }
        FString Result = *Line + Index;
        return Result.Replace(TEXT("@"), TEXT("#"));
    }

    // parse multiple line comments
    void ExportMultiLineComments(const FString &Comment, FString &GeneratedFileContent)
    {
        if (Comment.Len() > 0)
        {
            TArray<FString> CommentLines;
            Comment.ParseIntoArray(CommentLines, TEXT("\n"), true);
            for (int32 i = 0; i < CommentLines.Num(); ++i)
            {
                FString Line = GetValidComment(CommentLines[i]);
                if (Line.Len() < 1)
                {
                    continue;
                }
                GeneratedFileContent += FString::Printf(TEXT("---%s\r\n"), *Line);
            }
        }
    }

    // export comment for a UPROPERTY
    void ExportPropertyComment(const FString &Comment, FString &GeneratedFileContent)
    {
        GeneratedFileContent += TEXT(" @");

        TArray<FString> CommentLines;
        Comment.ParseIntoArray(CommentLines, TEXT("\n"), true);
        for (int32 i = 0; i < CommentLines.Num(); ++i)
        {
            FString Line = GetValidComment(CommentLines[i]);
            int32 N = Line.Len();
            if (N < 1)
            {
                continue;
            }
            GeneratedFileContent += Line;
            if (Line[N-1] != ',' && Line[N - 1] != '.')
            {
                GeneratedFileContent += TEXT(".");
            }
            GeneratedFileContent += TEXT(" ");
        }
    }

    void SaveFile(const FString &ModuleName, const FString &FileName, const FString &GeneratedFileContent)
    {
        IFileManager &FileManager = IFileManager::Get();
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
            check(bResult);
        }
    }

    // get readable type name for a UPROPERTY
    FString GetTypeName(FProperty *Property)
    {
        // #lizard forgives

        check(Property);

        if (Property)
        {
            if (const FByteProperty *TempByteProperty = CastField<FByteProperty>(Property))
            {
                return TEXT("integer");
            }
            else if (const FInt8Property *TempI8Property = CastField<FInt8Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FInt16Property *TempI16Property = CastField<FInt16Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FIntProperty *TempI32Property = CastField<FIntProperty>(Property))
            {
                return TEXT("integer");
            }
            else if (const FInt64Property *TempI64Property = CastField<FInt64Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FUInt16Property *TempU16Property = CastField<FUInt16Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FUInt32Property *TempU32Property = CastField<FUInt32Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FUInt64Property *TempU64Property = CastField<FUInt64Property>(Property))
            {
                return TEXT("integer");
            }
            else if (const FFloatProperty *TempFloatProperty = CastField<FFloatProperty>(Property))
            {
                return TEXT("number");
            }
            else if (const FDoubleProperty *TempDoubleProperty = CastField<FDoubleProperty>(Property))
            {
                return TEXT("number");
            }
            else if (const FEnumProperty *TempEnumProperty = CastField<FEnumProperty>(Property))
            {
                return ((FEnumProperty*)Property)->GetEnum()->GetName();
            }
            else if (const FBoolProperty *TempBoolProperty = CastField<FBoolProperty>(Property))
            {
                return TEXT("boolean");
            }
            else if (const FClassProperty *TempClassProperty = CastField<FClassProperty>(Property))
            {
                UClass *Class = ((FClassProperty*)Property)->MetaClass;
                return FString::Printf(TEXT("TSubclassOf<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FSoftObjectProperty *TempSoftObjectProperty = CastField<FSoftObjectProperty>(Property))
            {
                if (((FSoftObjectProperty*)Property)->PropertyClass->IsChildOf(UClass::StaticClass()))
                {
                    UClass *Class = ((FSoftClassProperty*)Property)->MetaClass;
                    return FString::Printf(TEXT("TSoftClassPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
                }
                UClass *Class = ((FSoftObjectProperty*)Property)->PropertyClass;
                return FString::Printf(TEXT("TSoftObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FObjectProperty *TempObjectProperty = CastField<FObjectProperty>(Property))
            {
                UClass *Class = ((FObjectProperty*)Property)->PropertyClass;
                return FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FWeakObjectProperty *TempWeakObjectProperty = CastField<FWeakObjectProperty>(Property))
            {
                UClass *Class = ((FWeakObjectProperty*)Property)->PropertyClass;
                return FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FLazyObjectProperty *TempLazyObjectProperty = CastField<FLazyObjectProperty>(Property))
            {
                UClass *Class = ((FLazyObjectProperty*)Property)->PropertyClass;
                return FString::Printf(TEXT("TLazyObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FInterfaceProperty *TempInterfaceProperty = CastField<FInterfaceProperty>(Property))
            {
                UClass *Class = ((FInterfaceProperty*)Property)->InterfaceClass;
                return FString::Printf(TEXT("TScriptInterface<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
            }
            else if (const FNameProperty *TempNameProperty = CastField<FNameProperty>(Property))
            {
                return TEXT("string");
            }
            else if (const FStrProperty *TempStringProperty = CastField<FStrProperty>(Property))
            {
                return TEXT("string");
            }
            else if (const FTextProperty *TempTextProperty = CastField<FTextProperty>(Property))
            {
                return TEXT("string");
            }
            else if (const FArrayProperty *TempArrayProperty = CastField<FArrayProperty>(Property))
            {
                FProperty *Inner = ((FArrayProperty*)Property)->Inner;
                return FString::Printf(TEXT("TArray<%s>"), *GetTypeName(Inner));
            }
            else if (const FMapProperty *TempMapProperty = CastField<FMapProperty>(Property))
            {
                FProperty *KeyProp = ((FMapProperty*)Property)->KeyProp;
                FProperty *ValueProp = ((FMapProperty*)Property)->ValueProp;
                return FString::Printf(TEXT("TMap<%s, %s>"), *GetTypeName(KeyProp), *GetTypeName(ValueProp));
            }
            else if (const FSetProperty *TempSetProperty = CastField<FSetProperty>(Property))
            {
                FProperty *ElementProp = ((FSetProperty*)Property)->ElementProp;
                return FString::Printf(TEXT("TSet<%s>"), *GetTypeName(ElementProp));
            }
            else if (const FStructProperty *TempStructProperty = CastField<FStructProperty>(Property))
            {
                return ((FStructProperty*)Property)->Struct->GetStructCPPName();
            }
            else if (const FDelegateProperty *TempDelegateProperty = CastField<FDelegateProperty>(Property))
            {
                return TEXT("Delegate");
            }
            else if (const FMulticastDelegateProperty *TempMulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property))
            {
                return TEXT("MulticastDelegate");
            }
        }

        return TEXT("Unknown");
    }

    FString GetModuleName(UPackage *Package)
    {
        check(Package);
        FString ModuleName = Package->GetName();
        int32 Index = INDEX_NONE;
        ModuleName.FindLastChar('/', Index);
        if (Index != INDEX_NONE)
        {
            return *ModuleName + Index + 1;
        }
        return ModuleName;
    }

    FString OutputDir;
    FString UE4Namespace;
    TArray<FString> UE4Names;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnLuaIntelliSenseModule, UnLuaIntelliSense)
