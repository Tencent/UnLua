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
#include "SocketSubsystem.h"
#include "UnLua.h"
#include "UnLuaEditorSettings.h"
#include "UnLuaFunctionLibrary.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPluginManager.h"

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

void UUnLuaEditorFunctionLibrary::FetchNewVersion()
{
    Async(EAsyncExecution::ThreadPool, []
    {
        const auto RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        bool bIsValid;
        RemoteAddr->SetIp(TEXT("101.226.141.148"), bIsValid);
        RemoteAddr->SetPort(8080);

        const auto Client = FUdpSocketBuilder(TEXT("Client")).Build();
        if (!Client->Connect(RemoteAddr.Get()))
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Unable to fetch latest version, could not connect to server."));
            return;
        }

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        const FString Msg = FString::Printf(TEXT("cmd=3&tag=glcoud.unlua.update&version=%s&engine=%s&user_name=%s"),
                                            *GetCurrentVersion(),
                                            *FEngineVersion::Current().ToString(),
                                            *FPlatformMisc::GetHashedMacAddressString()
        );
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        const FTCHARToUTF8 Converted(*Msg);
        int32 Sent;
        if (!Client->Send((uint8*)Converted.Get(), Converted.Length(), Sent))
        {
            UE_LOG(LogUnLua, Warning, TEXT("Unable to fetch latest version, data sending error."));
            Client->Close();
            return;
        }

        TArray<uint8> Bytes;
        Bytes.SetNumZeroed(256);
        int32 Read = 0;
        for (int Tried = 0; Tried < 10; ++Tried)
        {
            FPlatformProcess::Sleep(0.5f);
            if (Client->Recv(Bytes.GetData(), Bytes.Num(), Read))
                break;
        }

        Client->Close();
        if (Read == 0)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Unable to fetch latest version, data receiving timeout."));
            return;
        }

        const FString Received = UTF8_TO_TCHAR((char*)Bytes.GetData());
        const FString CurrentVersion = GetCurrentVersion();
        if (Received == CurrentVersion)
            return;
        
        UE_LOG(LogUnLua, Log, TEXT("New version available: %s."), *Received);
    });
}

FString UUnLuaEditorFunctionLibrary::GetCurrentVersion()
{
    auto& PluginManager = IPluginManager::Get();
    auto Plugins = PluginManager.GetDiscoveredPlugins();
    for (const auto& Plugin : Plugins)
    {
        if (Plugin->GetName() == "UnLua")
        {
            const auto Descriptor = Plugin->GetDescriptor();
            return Descriptor.VersionName;
        }
    }
    return "";
}

void UUnLuaEditorFunctionLibrary::OnLuaFilesModified(const TArray<FFileChangeData>& FileChanges)
{
    const auto& Settings = *GetDefault<UUnLuaEditorSettings>();
    if (Settings.HotReloadMode != EHotReloadMode::Auto)
        return;
    UUnLuaFunctionLibrary::HotReload();
}

FDelegateHandle UUnLuaEditorFunctionLibrary::DirectoryWatcherHandle;
