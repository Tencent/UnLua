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


#include "DirectoryWatcherModule.h"
#include "EditorHotReloadWatcher.h"

namespace UnLua
{
    FEditorHotReloadWatcher::FEditorHotReloadWatcher(FLuaEnv* Env)
        : Env(Env)
    {
        Env->OnModuleLoaded.AddRaw(this, &FEditorHotReloadWatcher::OnModuleLoaded);
    }

    void FEditorHotReloadWatcher::Watch(const TArray<FString>& Directories)
    {
        auto& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");
        const auto DirectoryWatcher = DirectoryWatcherModule.Get();
        if (!DirectoryWatcher)
            return;

        for (const auto& Watcher : Watchers)
            DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Watcher.Directory, Watcher.Handle);
        Watchers.Reset();

        for (const auto& Directory : Directories)
        {
            if (!FPaths::DirectoryExists(Directory))
                continue;
            FDirectoryWatcherPayload Watcher;
            Watcher.Directory = Directory;
            const auto& Delegate = IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FEditorHotReloadWatcher::OnFileChanged);
            DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(Watcher.Directory, Delegate, Watcher.Handle);
        }
    }

    void FEditorHotReloadWatcher::OnModuleLoaded(const FString& ModuleName, const FString& FullPath)
    {
        PathToModuleName.Add(FullPath, ModuleName);
    }

    void FEditorHotReloadWatcher::OnFileChanged(const TArray<FFileChangeData>& FileChanges)
    {
        TArray<FString> ToReload;
        for (auto& FileChange : FileChanges)
        {
            if (FileChange.Action != FFileChangeData::FCA_Modified)
                continue;

            const auto ModuleName = PathToModuleName.Find(FileChange.Filename);
            if (!ModuleName)
                continue;
            ToReload.AddUnique(*ModuleName);
        }
        Env->HotReload(ToReload);
    }
}
