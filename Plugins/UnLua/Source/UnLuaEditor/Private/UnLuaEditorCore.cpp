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

        FString Content;
        Content += FString::Printf(TEXT("require \"UnLua\"\r\n\r\n"));
        Content += FString::Printf(TEXT("local %s = Class()\r\n\r\n"), *ClassName);
        Content += FString::Printf(TEXT("--function %s:Initialize(Initializer)\r\n"), *ClassName);
        Content += FString::Printf(TEXT("--end\r\n\r\n"));

        if (Class->IsChildOf(AActor::StaticClass()))
        {
            // default BlueprintEvents for Actor

            Content += FString::Printf(TEXT("--function %s:UserConstructionScript()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveBeginPlay()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveEndPlay()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveTick(DeltaSeconds)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveActorBeginOverlap(OtherActor)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveActorEndOverlap(OtherActor)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));
        }
        else if (Class->IsChildOf(UUserWidget::StaticClass()))
        {
            // default BlueprintEvents for UserWidget (UMG)

            Content += FString::Printf(TEXT("--function %s:PreConstruct(IsDesignTime)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:Construct()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:Tick(MyGeometry, InDeltaTime)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));
        }
        else if (Class->IsChildOf(UAnimInstance::StaticClass()))
        {
            // default BlueprintEvents for AnimInstance (animation blueprint)

            Content += FString::Printf(TEXT("--function %s:BlueprintInitializeAnimation()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:BlueprintBeginPlay()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:BlueprintUpdateAnimation(DeltaTimeX)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:BlueprintPostEvaluateAnimation()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));
        }
        else if (Class->IsChildOf(UActorComponent::StaticClass()))
        {
            // default BlueprintEvents for ActorComponent

            Content += FString::Printf(TEXT("--function %s:ReceiveBeginPlay()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveEndPlay()\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));

            Content += FString::Printf(TEXT("--function %s:ReceiveTick(DeltaSeconds)\r\n"), *ClassName);
            Content += FString::Printf(TEXT("--end\r\n\r\n"));
        }

        Content += FString::Printf(TEXT("return %s\r\n"), *ClassName);

        FString FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *ClassName);
        return FFileHelper::SaveStringToFile(Content, *FileName);
    }
    return false;
}
