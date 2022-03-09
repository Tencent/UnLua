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
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnLua.h"

FString UUnLuaEditorFunctionLibrary::GetScriptRootPath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Script/"));
}

int64 UUnLuaEditorFunctionLibrary::GetFileLastModifiedTimestamp(FString Path)
{
	const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*Path);
	return FileTime.GetTicks();
}

void UUnLuaEditorFunctionLibrary::WatchScriptDirectory()
{
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
	if (DirectoryWatcher)
	{
		const auto& Delegate = IDirectoryWatcher::FDirectoryChanged::CreateStatic(&UUnLuaEditorFunctionLibrary::OnLuaFilesModified);
		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(GetScriptRootPath(), Delegate, DirectoryWatcherHandle);
	}
}

void UUnLuaEditorFunctionLibrary::OnLuaFilesModified(const TArray<FFileChangeData>& FileChanges)
{
	TArray<FString> Added;
	TArray<FString> Modified;
	TArray<FString> Removed;
        
	for (auto& FileChange : FileChanges)
	{
		const auto ModuleName = FileChange.Filename.Replace(TEXT("/"), TEXT(".")).Replace(TEXT("\\"), TEXT("."));
		switch (FileChange.Action)
		{
		case FFileChangeData::FCA_Added:
			Added.AddUnique(ModuleName);
			break;
		case FFileChangeData::FCA_Modified:
			Modified.AddUnique(ModuleName);
			break;
		case FFileChangeData::FCA_Removed:
			Removed.AddUnique(ModuleName);
			break;
		default:
			break;
		}
	}

	lua_State* L = UnLua::GetState();
	if (L)
	{
		// TODO:refactor with UnLua::GetEnvGroup FLuaEnv
		UnLua::Call(L, "UnLuaHotReload", Added, Modified, Removed);
	}
}

FDelegateHandle UUnLuaEditorFunctionLibrary::DirectoryWatcherHandle;