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

#include "UnLuaPrivate.h"
#include "Misc/FileHelper.h"
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IPluginManager.h"

// copy dependency file to plugin's content dir
static bool CopyDependencyFile(const TCHAR *FileName)
{
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();
    FString SrcFilePath = ContentDir / FileName;
    FString DestFilePath = GLuaSrcFullPath / FileName;
    bool bSuccess = IFileManager::Get().FileExists(*DestFilePath);
    if (!bSuccess)
    {
        bSuccess = IFileManager::Get().FileExists(*SrcFilePath);
        if (!bSuccess)
        {
            return false;
        }

        uint32 CopyResult = IFileManager::Get().Copy(*DestFilePath, *SrcFilePath, 1, true);
        if (CopyResult != COPY_OK)
        {
            return false;
        }
    }
    return true;
}

// create Lua template file for the selected blueprint
bool CreateLuaTemplateFile(UBlueprint *Blueprint)
{
    if (Blueprint)
    {
        // copy dependency file first
        if (!CopyDependencyFile(TEXT("UnLua.lua")))
        {
            return false;
        }

        UClass *Class = Blueprint->GeneratedClass;
        FString ClassName = Class->GetName();
        FString FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *ClassName);
        if (FPaths::FileExists(FileName))
        {
            UE_LOG(LogUnLua, Warning, TEXT("Lua file (%s) is already existed!"), *ClassName);
            return false;
        }

        static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();

        FString TemplateName;
        if (Class->IsChildOf(AActor::StaticClass()))
        {
            // default BlueprintEvents for Actor
            TemplateName = ContentDir + TEXT("/ActorTemplate.lua");
        }
        else if (Class->IsChildOf(UUserWidget::StaticClass()))
        {
            // default BlueprintEvents for UserWidget (UMG)
            TemplateName = ContentDir + TEXT("/UserWidgetTemplate.lua");
        }
        else if (Class->IsChildOf(UAnimInstance::StaticClass()))
        {
            // default BlueprintEvents for AnimInstance (animation blueprint)
            TemplateName = ContentDir + TEXT("/AnimInstanceTemplate.lua");
        }
        else if (Class->IsChildOf(UActorComponent::StaticClass()))
        {
            // default BlueprintEvents for ActorComponent
            TemplateName = ContentDir + TEXT("/ActorComponentTemplate.lua");
        }

        FString Content;
        FFileHelper::LoadFileToString(Content, *TemplateName);
        Content = Content.Replace(TEXT("TemplateName"), *ClassName);

        return FFileHelper::SaveStringToFile(Content, *FileName);
    }
    return false;
}

