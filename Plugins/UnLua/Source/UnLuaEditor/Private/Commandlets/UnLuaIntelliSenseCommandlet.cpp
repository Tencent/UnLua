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

#include "Commandlets/UnLuaIntelliSenseCommandlet.h"
#include "LuaContext.h"

UUnLuaIntelliSenseCommandlet::UUnLuaIntelliSenseCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Context(nullptr)
{
    //IntermediateDir = FPaths::ProjectIntermediateDir();
    IntermediateDir = FPaths::ProjectPluginsDir();
    IntermediateDir += TEXT("UnLua/Intermediate/");
}

int32 UUnLuaIntelliSenseCommandlet::Main(const FString &Params)
{
    // class black list
    TArray<FName> ClassBlackList;
    ClassBlackList.Add("int8");
    ClassBlackList.Add("int16");
    ClassBlackList.Add("int32");
    ClassBlackList.Add("int64");
    ClassBlackList.Add("uint8");
    ClassBlackList.Add("uint16");
    ClassBlackList.Add("uint32");
    ClassBlackList.Add("uint64");
    ClassBlackList.Add("float");
    ClassBlackList.Add("double");
    ClassBlackList.Add("bool");
    ClassBlackList.Add("FName");
    ClassBlackList.Add("FString");
    ClassBlackList.Add("FText");

    // enum black list
    TArray<FString> EnumBlackList;

    // function black list
    TArray<FString> FuncBlackList;
    FuncBlackList.Add("OnModuleHotfixed");

    Context = FLuaContext::Create();
    const TMap<FName, UnLua::IExportedClass*> &ExportedReflectedClasses = Context->GetExportedReflectedClasses();
    const TMap<FName, UnLua::IExportedClass*> &ExportedNonReflectedClasses = Context->GetExportedNonReflectedClasses();
    const TArray<UnLua::IExportedEnum*> &ExportedEnums = Context->GetExportedEnums();
    const TArray<UnLua::IExportedFunction*> &ExportedFunctions = Context->GetExportedFunctions();

    FString GeneratedFileContent;
    FString ModuleName(TEXT("StaticallyExports"));

    // reflected classes
    for (TMap<FName, UnLua::IExportedClass*>::TConstIterator It(ExportedReflectedClasses); It; ++It)
    {
        if (ClassBlackList.Find(It.Key()) != INDEX_NONE)
        {
            continue;
        }

        It.Value()->GenerateIntelliSense(GeneratedFileContent);
        SaveFile(ModuleName, It.Key().ToString(), GeneratedFileContent);
    }

    // non-reflected classes
    for (TMap<FName, UnLua::IExportedClass*>::TConstIterator It(ExportedNonReflectedClasses); It; ++It)
    {
        if (ClassBlackList.Find(It.Key()) != INDEX_NONE)
        {
            continue;
        }

        It.Value()->GenerateIntelliSense(GeneratedFileContent);
        SaveFile(ModuleName, It.Key().ToString(), GeneratedFileContent);
    }

    // enums
    for (UnLua::IExportedEnum *Enum : ExportedEnums)
    {
        if (EnumBlackList.Find(Enum->GetName()) != INDEX_NONE)
        {
            continue;
        }

        Enum->GenerateIntelliSense(GeneratedFileContent);
        SaveFile(ModuleName, Enum->GetName(), GeneratedFileContent);
    }

    // global functions
    GeneratedFileContent.Empty();
    for (UnLua::IExportedFunction *Function : ExportedFunctions)
    {
        if (FuncBlackList.Find(Function->GetName()) != INDEX_NONE)
        {
            continue;
        }

        Function->GenerateIntelliSense(GeneratedFileContent);
    }
    SaveFile(ModuleName, TEXT("GlobalFunctions"), GeneratedFileContent);

    return 0;
}

void UUnLuaIntelliSenseCommandlet::SaveFile(const FString &ModuleName, const FString &FileName, const FString &GeneratedFileContent)
{
    IFileManager &FileManager = IFileManager::Get();
    FString Directory = FString::Printf(TEXT("%sIntelliSense/%s"), *IntermediateDir, *ModuleName);
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
