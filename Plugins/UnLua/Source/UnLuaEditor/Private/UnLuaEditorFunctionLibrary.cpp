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


#include "UnLuaEditorFunctionLibrary.h"
#include "Modules/ModuleManager.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "UnLua.h"
#include "UnLuaEditorSettings.h"
#include "UnLuaFunctionLibrary.h"

void UUnLuaEditorFunctionLibrary::WatchScriptDirectory()
{
    FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");
    IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
    if (DirectoryWatcher)
    {
        const auto& Delegate = IDirectoryWatcher::FDirectoryChanged::CreateStatic(&UUnLuaEditorFunctionLibrary::OnLuaFilesModified);
        const auto& ScriptRootPath = UUnLuaFunctionLibrary::GetScriptRootPath();
        DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(ScriptRootPath, Delegate, DirectoryWatcherHandle);
    }
}

void UUnLuaEditorFunctionLibrary::OnLuaFilesModified(const TArray<FFileChangeData>& FileChanges)
{
    const auto& Settings = *GetDefault<UUnLuaEditorSettings>();
    if (Settings.HotReloadMode == EHotReloadMode::Auto)
        return;
    UUnLuaFunctionLibrary::HotReload();
}

FDelegateHandle UUnLuaEditorFunctionLibrary::DirectoryWatcherHandle;
