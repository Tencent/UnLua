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

#if WITH_DEV_AUTOMATION_TESTS

#include "TestCommands.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#include "LevelEditor.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "Tests/AutomationEditorCommon.h"
#endif
#endif

namespace UnLuaTestSuite
{
    FOpenMapLatentCommand::FOpenMapLatentCommand(FString MapName, bool bForceReload)
        : MapName(MapName), bForceReload(bForceReload)
    {
    }

    bool FOpenMapLatentCommand::Update()
    {
        if (WaitForMapToLoad)
            return WaitForMapToLoad->Update();

        LoadMap();
        return false;
    }

    void FOpenMapLatentCommand::LoadMap()
    {
#if WITH_EDITOR
        struct FFailedGameStartHandler
        {
            bool bCanProceed;

            FFailedGameStartHandler()
            {
                bCanProceed = true;
                FEditorDelegates::EndPIE.AddRaw(this, &FFailedGameStartHandler::OnEndPIE);
            }

            ~FFailedGameStartHandler()
            {
                FEditorDelegates::EndPIE.RemoveAll(this);
            }

            bool CanProceed() const { return bCanProceed; }

            void OnEndPIE(const bool bInSimulateInEditor)
            {
                bCanProceed = false;
            }
        };

        bool bNeedLoadEditorMap = true;
        bool bPieRunning = false;

        //check existing worlds
        const TIndirectArray<FWorldContext> WorldContexts = GEngine->GetWorldContexts();
        for (auto& Context : WorldContexts)
        {
            if (Context.World())
            {
                FString WorldPackage = Context.World()->GetOutermost()->GetName();

                if (Context.WorldType == EWorldType::PIE)
                {
                    //don't quit!  This was triggered while pie was already running!
                    bPieRunning = true;
                    break;
                }
                else if (Context.WorldType == EWorldType::Editor)
                {
                    bNeedLoadEditorMap = MapName != WorldPackage;
                }
            }
        }

        if (bNeedLoadEditorMap || bForceReload)
        {
            if (bPieRunning)
                GEditor->EndPlayMap();
            FEditorFileUtils::LoadMap(*MapName, false, true);
        }

        FFailedGameStartHandler FailHandler;

        FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

        FRequestPlaySessionParams RequestParams;
        ULevelEditorPlaySettings* EditorPlaySettings = NewObject<ULevelEditorPlaySettings>();
        EditorPlaySettings->SetPlayNumberOfClients(1);
        EditorPlaySettings->bLaunchSeparateServer = false;
        RequestParams.EditorPlaySettings = EditorPlaySettings;
        RequestParams.DestinationSlateViewport = ActiveLevelViewport;

#if ENGINE_MAJOR_VERSION >= 5
        // Make sure the player start location is a valid location.
        if (GEditor->CheckForPlayerStart() == nullptr)
        {
            FAutomationEditorCommonUtils::SetPlaySessionStartToActiveViewport(RequestParams);
        }
#endif

        GEditor->RequestPlaySession(RequestParams);

        // Immediately launch the session 
        GEditor->StartQueuedPlaySessionRequest();

        check(FailHandler.CanProceed());
        WaitForMapToLoad = MakeUnique<FWaitForMapToLoadCommand>();
#endif
    }

    bool FEndPlayMapCommand::Update()
    {
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION < 5
        if (GEditor && GEditor->PlayWorld)
            GEditor->EndPlayMap();
#endif
#endif
        return true;
    }
}

#endif
