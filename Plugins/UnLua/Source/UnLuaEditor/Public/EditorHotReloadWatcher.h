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


#pragma once

#include "IDirectoryWatcher.h"
#include "IHotReloadWatcher.h"
#include "LuaEnv.h"

namespace UnLua
{
    class FEditorHotReloadWatcher : public IHotReloadWatcher
    {
    public:
        explicit FEditorHotReloadWatcher(FLuaEnv* Env);

        virtual void Watch(const TArray<FString>& Directories) override;

    protected:
        FLuaEnv* Env;

    private:
        void OnModuleLoaded(const FString& ModuleName, const FString& FullPath);

        void OnFileChanged(const TArray<FFileChangeData>& FileChanges);

        struct FDirectoryWatcherPayload
        {
            FString Directory;
            FDelegateHandle Handle;
        };

        TArray<FDirectoryWatcherPayload> Watchers;
        TMap<FString, FString> PathToModuleName;
    };
}
