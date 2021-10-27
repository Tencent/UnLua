// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LuaContext.h"
#include "UnLuaDelegates.h"
#include "lua.hpp"
#include "UnLua.h"
#include "UnLuaBase.h"
#include "UnLuaPrivate.h"
#include "LuaCore.h"
#include "UnLuaFrameWork.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/PlatformOutputDevices.h"
#include "Misc/App.h"
#include "Misc/Parse.h"

#include "Kismet/KismetSystemLibrary.h"

#include <time.h>
#if PLATFORM_ANDROID || PLATFORM_IOS
#include <sys/time.h>
#include <pthread.h>
#elif PLATFORM_WINDOWS
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#define LOCTEXT_NAMESPACE "UnLuaFrameWorkModule"

// 返回真正要读取的文件路径，沙盒优先,返回true则是读取的沙盒文件
bool UnLua::GetRealFilePath(const FString& FilePath, FString& RealFilePath)
{
	RealFilePath = "";

	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	if (!ProjectDir.EndsWith("/"))
	{
		ProjectDir.Append("/");
	}
	FString ProjectPersistentDownloadDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPersistentDownloadDir());
	if (!ProjectPersistentDownloadDir.EndsWith("/"))
	{
		ProjectPersistentDownloadDir.Append("/");
	}
	RealFilePath = FilePath.Replace(*ProjectDir, *ProjectPersistentDownloadDir);
	if ((!RealFilePath.Contains(*ProjectPersistentDownloadDir))
		|| (!IFileManager::Get().FileExists(*RealFilePath)))
	{
		RealFilePath = FilePath;
		return false;
	}
	return true;
}

int32 Global_LoadLuaFile(lua_State *L)
{
	// Lua Full Path
	const char* LuaPath = lua_tostring(L, 1);
	if (LuaPath)
	{
		FString FullLuaPath = GLuaSrcFullPath + UTF8_TO_TCHAR(LuaPath);
		int nargs = lua_gettop(L);
		bool LoadResult = false;
		if (1 == nargs)
		{
			LoadResult = UnLua::LoadFile(L, UTF8_TO_TCHAR(LuaPath));// ::LoadLuaFile(L, *FullLuaPath);
		}
		else
		{
			// Mode,Env From Lua 
			const char* Mode = lua_tostring(L, 2);
			int Env = (!lua_isnone(L, 3) ? 3 : 0);
			return UnLua::LoadFile(L, UTF8_TO_TCHAR(LuaPath), Mode, Env);
			//return ::LoadLuaFile(L, *FullLuaPath, Mode, Env);
		}

		if (!LoadResult)
		{
			lua_settop(L, 0);
			lua_pushnil(L);
			lua_pushfstring(L, "LoadLuaFile error");
			return 2;
		}

		return 1;
	}
	else
	{
		lua_settop(L, 0);
		lua_pushnil(L);
		lua_pushfstring(L, "Invalid LuaFileName");

		return 2;
	}
}

int32 Global_GetFileDateTime(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (1 <= nargs)
	{
		// Lua Full Path
		const char* FilePath = lua_tostring(L, 1);
		if (FilePath)
		{
			FString RealFilePath = "";
			bool bSandBoxFile = UnLua::GetRealFilePath(UTF8_TO_TCHAR(FilePath), RealFilePath);
			FDateTime FileTime = IFileManager::Get().GetTimeStamp(*RealFilePath);

			if (FileTime == FDateTime::MinValue())
			{
				//AndroidFile.cpp GetTimeStamp 没有考虑读取沙盒文件
#if PLATFORM_ANDROID
				if (bSandBoxFile)
				{
					const FDateTime AndroidEpoch(1970, 1, 1);
					struct stat FileInfo;
					if (stat(TCHAR_TO_UTF8(*RealFilePath), &FileInfo) == -1)
					{
						UE_LOG(LogUnLua, Log, TEXT("stat Error : %s"), *RealFilePath);
					}
					else
					{
						FTimespan TimeSinceEpoch(0, 0, FileInfo.st_mtime);
						FileTime = AndroidEpoch + TimeSinceEpoch;

						//UE_LOG(LogUnLua, Log, TEXT("stat success : %d"), FileInfo.st_mtime);
					}
				}
#else
				UE_LOG(LogUnLua, Log, TEXT("GetFileTime Error : %s, Request Path : %s"), *RealFilePath, ANSI_TO_TCHAR(FilePath));
#endif		
			}
			UnLua::Push(L, FileTime.GetTicks());
			return 1;
		}
	}

	return 0;
}

int32 Global_IsFileExists(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (1 <= nargs)
	{
		// Lua Full Path
		const char* FilePath = lua_tostring(L, 1);
		if (FilePath)
		{
			FString RealFilePath = "";
			UnLua::GetRealFilePath(UTF8_TO_TCHAR(FilePath), RealFilePath);
			bool FileExists = IFileManager::Get().FileExists(*RealFilePath);
			UnLua::Push(L, FileExists);
			return 1;
		}
	}

	return 0;
}

int32 Global_IsDirctoryExists(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (1 <= nargs)
	{
		// Lua Full Path
		const char* Path = lua_tostring(L, 1);
		if (Path)
		{
			bool DirExists = IFileManager::Get().DirectoryExists(UTF8_TO_TCHAR(Path));
			UnLua::Push(L, DirExists);
			return 1;
		}
	}

	return 0;
}

int32 Global_CreateDirctory(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (1 <= nargs)
	{
		// Lua Full Path
		const char* Path = lua_tostring(L, 1);
		if (Path)
		{
			bool DirExists = IFileManager::Get().MakeDirectory(UTF8_TO_TCHAR(Path), true);
			UnLua::Push(L, DirExists);
			return 1;
		}
	}

	return 0;
}

int32 Global_LoadRawFile(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (1 <= nargs)
	{
		// Lua Full Path
		const char* FilePath = lua_tostring(L, 1);
		if (FilePath)
		{
			if (1 == nargs)
			{
				FString RealFilePath = "";
				if (UnLua::GetRealFilePath(UTF8_TO_TCHAR(FilePath), RealFilePath))
				{
					UE_LOG(LogUnLua, Log, TEXT("Load Raw file from DownloadDir : %s, Request Path : %s"), *RealFilePath, UTF8_TO_TCHAR(FilePath));
				}

				TArray<uint8> RawData;
				bool LoadResult = FFileHelper::LoadFileToArray(RawData, *RealFilePath, 0);
				if (LoadResult)
				{
					UnLua::FlStringWrapper LWrapper;
					LWrapper.V = (const char*)(RawData.GetData());
					LWrapper.Len = (uint32)RawData.Num();
					UnLua::Push(L, LWrapper);
				}
			}

			return 1;
		}
	}

	return 0;
}

int32 Global_EnumDir(lua_State *L)
{
	if (2 == lua_gettop(L))
	{
		const char* DirPath = lua_tostring(L, 1);
		const char* FileExt = lua_tostring(L, 2);
		if ((DirPath)
			&& (FileExt))
		{
			TArray<FString> FindedFiles;
			IFileManager::Get().FindFilesRecursive(FindedFiles, UTF8_TO_TCHAR(DirPath), UTF8_TO_TCHAR(FileExt), true, false);

			lua_newtable(L);
			for (int i = 0; i < FindedFiles.Num(); i++)
			{
				lua_pushinteger(L, i);
				lua_pushstring(L, TCHAR_TO_UTF8(*FindedFiles[i]));
				lua_rawset(L, -3);
			}

			return 1;
		}
	}
	return 0;
}

static FString _LogPrefix(const FName& Category)
{
#if NO_LOGGING
	return FString(TEXT(""));
#else

	const int32 BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE] = { 0 };

	time_t now;
	time(&now);
	struct tm local = *localtime(&now);
	struct timeval tvl;
#if PLATFORM_WINDOWS
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tvl.tv_sec = mktime(&local);
	tvl.tv_usec = wtm.wMilliseconds;
#else
	gettimeofday(&tvl, 0);
#endif

	if (Category == LogUnLua.GetCategoryName())
	{
		snprintf(buffer, BUFFER_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d.%03d]",
			local.tm_year + 1900,
			local.tm_mon + 1,
			local.tm_mday,
			local.tm_hour,
			local.tm_min,
			local.tm_sec,
			(int)tvl.tv_usec
		);
	}
	else
	{
#if PLATFORM_WINDOWS
		snprintf(buffer, BUFFER_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][%s][%u]",
#else
		snprintf(buffer, BUFFER_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][%s][%lu]",
#endif
			local.tm_year + 1900,
			local.tm_mon + 1,
			local.tm_mday,
			local.tm_hour,
			local.tm_min,
			local.tm_sec,
			(int)tvl.tv_usec,
			TCHAR_TO_UTF8(*Category.ToString()),
#if PLATFORM_WINDOWS
			(unsigned int)GetCurrentThreadId()
#else
			(unsigned long)pthread_self()
#endif
		);
	}

	return FString(buffer);

#endif
}

int32 Global_UEPrint(lua_State *L)
{
#if ! NO_LOGGING
	FString LogPrefix;
	FString StrLog;
	FString LogLevelName[] = {/*0*/FString("[Trace]"), /*1*/FString("[Debug]"), /*2*/FString("[Info]"), /*3*/FString("[Warn]"), /*4*/FString("[Error]"), /*5*/FString("[Fatal]") };
	int32 nargs = lua_gettop(L);

	if (nargs < 3)
	{
		UE_LOG(LogUnLua, Error, TEXT("Global_Print error parameters!"));
		return 0;
	}

	int32 loglevel = lua_tointeger(L, 1);
	const char* logPrefix = lua_tostring(L, 2);
	const char* source = lua_tostring(L, 3);
	if (loglevel < 0 || loglevel > 5)
	{
		UE_LOG(LogUnLua, Error, TEXT("Global_Print error loglevel=%d"), loglevel);
		return 0;
	}
	LogPrefix += _LogPrefix(LogUnLua.GetCategoryName()) + FString("[LUA]") + LogLevelName[loglevel] + FString("[") + UTF8_TO_TCHAR(source) + FString("] ");

	for (int32 i = 4; i <= nargs; ++i)
	{
		const char* arg = luaL_tolstring(L, i, nullptr);
		if (!arg)
		{
			arg = "";
		}
		StrLog += UTF8_TO_TCHAR(arg);
		StrLog += TEXT("  ");
	}

	UE_LOG(LogUnLua, Log, TEXT("%s"), *(LogPrefix + StrLog));
	if (IsInGameThread())
	{
		UKismetSystemLibrary::PrintString(GWorld, StrLog, false, false);
	}
#endif
	return 0;
}

void UnLuaFrame_OnLuaStateCreate(lua_State* L)
{
#if UNLUA_ENABLE_AUTO_HOTFIX
	UnLua::FAutoStack StackKeeper;

	UnLua::RegisteG6Evn(L);

	if (UnLua::Call(L, "require", "G6HotfixHelper").IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("G6HotfixHelper Loaded Success"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("G6HotfixHelper Loaded failed"));
	}
#endif
}

void UnLuaFrame_OnLuaStateDestroy(bool bFullClean)
{
	if (bFullClean)
	{

	}
}

void UnLuaFrame_OnObjectBinded(UObjectBaseUtility* Object)
{
	lua_State* L = UnLua::GetState();
	if (L)
	{
		UnLua::Call(L, "OnObjectCreated", UnLua::FLuaIndex(-1));
	}
}

void UnLuaFrame_OnObjectUnbinded(UObjectBaseUtility* Object)
{
	lua_State* L = UnLua::GetState();
	if (L)
	{
		UnLua::Call(L, "OnObjectDestroyed", UnLua::FLuaIndex(-1));
	}
}


void UnLuaFrameWorkModule::StartupModule()
{
	FUnLuaDelegates::OnLuaStateCreated.AddStatic(&UnLuaFrame_OnLuaStateCreate);
	FUnLuaDelegates::OnPreLuaContextCleanup.AddStatic(&UnLuaFrame_OnLuaStateDestroy);
	
#if UNLUA_ENABLE_AUTO_HOTFIX
	TickDelegate = FTickerDelegate::CreateRaw(this, &UnLuaFrameWorkModule::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
#endif
}

void UnLuaFrameWorkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

#if UNLUA_ENABLE_AUTO_HOTFIX
	FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
#endif
}

#if UNLUA_ENABLE_AUTO_HOTFIX
bool UnLuaFrameWorkModule::Tick(float DeltaTime)
{
	lua_State* L = UnLua::GetState();
	if (L)
	{
		UnLua::Call(L, "HotFix", true);
	}
	return true;
}
#endif

namespace UnLua
{
	void WriteG6lib(lua_State* L)
	{
		if (L)
		{
			lua_newtable(L);

			lua_pushstring(L, "print");
			lua_pushcfunction(L, Global_UEPrint);
			lua_settable(L, -3);

			lua_pushstring(L, "EnumDir");
			lua_pushcfunction(L, Global_EnumDir);
			lua_settable(L, -3);

			lua_pushstring(L, "IsFileExists");
			lua_getglobal(L, "UEIsFileExists");
			lua_settable(L, -3);

			lua_pushstring(L, "GetFileDateTime");
			lua_getglobal(L, "UEGetFileDateTime");
			lua_settable(L, -3);

			lua_setglobal(L, "G6lib");
		}
	}

	void RegisteG6Evn(lua_State* L)
	{
		lua_register(L, "UELoadLuaFile", Global_LoadLuaFile);
		lua_register(L, "UELoadRawFile", Global_LoadRawFile);
		lua_register(L, "UEGetFileDateTime", Global_GetFileDateTime);
		lua_register(L, "UEIsFileExists", Global_IsFileExists);
		lua_register(L, "UEIsDirctoryExists", Global_IsDirctoryExists);
		lua_register(L, "UECreateDirctory", Global_CreateDirctory);

		//写入环境变量
		lua_getglobal(L, "G6ENV");
		bool bHaveG6ENV = true;
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			bHaveG6ENV = false;
			lua_newtable(L);
		}

		lua_pushstring(L, "USING_FLEXBUFFER");
#if USING_FLEXBUFFER
		lua_pushboolean(L, true);
#else
		lua_pushboolean(L, false);
#endif
		lua_settable(L, -3);

		lua_pushstring(L, "_PROJECT_ROOT");
		lua_pushstring(L, TCHAR_TO_UTF8(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())));
		lua_settable(L, -3);

        lua_pushstring(L, "_SANDBOX_PATH");
		lua_pushstring(L, TCHAR_TO_UTF8(*IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::ProjectPersistentDownloadDir())));
		
        lua_settable(L, -3);
        
		lua_pushstring(L, "_LUA_ROOT_PATH");
		lua_pushstring(L, TCHAR_TO_UTF8(*GLuaSrcFullPath));
		lua_settable(L, -3);

#if PLATFORM_IOS
		lua_pushstring(L, "_IOS_PRIVATE_PATH");
		lua_pushstring(L, TCHAR_TO_UTF8(*(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::ProjectDir()).Replace(UTF8_TO_TCHAR("/private/"), UTF8_TO_TCHAR("/")))));
		lua_settable(L, -3);
#endif

#if WITH_EDITOR
		lua_pushstring(L, "_ISEDITOR");
		lua_pushboolean(L, true);
		lua_settable(L, -3);
#endif

		lua_pushstring(L, "_PLATFORM");
#if PLATFORM_WINDOWS
		lua_pushstring(L, "PLATFORM_WINDOWS");
#elif PLATFORM_MAC
		lua_pushstring(L, "PLATFORM_MAC");
#elif PLATFORM_IOS
		lua_pushstring(L, "PLATFORM_IOS");
#elif PLATFORM_ANDROID
		lua_pushstring(L, "PLATFORM_ANDROID");
#elif PLATFORM_HTML5
		lua_pushstring(L, "PLATFORM_HTML5");
#elif PLATFORM_LINUX
		lua_pushstring(L, "PLATFORM_LINUX");
#else
		lua_pushstring(L, "PLATFORM_UNKOWN");
#endif
		lua_settable(L, -3);

		lua_pushstring(L, "_DEVICE_INFO");
		lua_pushfstring(L, TCHAR_TO_UTF8(*FGenericPlatformMisc::GetDeviceMakeAndModel()));
		lua_settable(L, -3);

		lua_pushstring(L, "_LOG_FILE");
#if PLATFORM_ANDROID
		//相对路径。安卓不在沙盒目录
		FString LogPath = FString(TEXT("/storage/emulated/0/UE4Game/")) + FApp::GetProjectName() + TEXT("/") + FApp::GetProjectName() + TEXT("/Saved/Logs/") + FApp::GetProjectName() + TEXT(".log");
		lua_pushstring(L, TCHAR_TO_UTF8(*LogPath));
#else
		lua_pushstring(L, TCHAR_TO_UTF8(*IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPlatformOutputDevices::GetAbsoluteLogFilename())));
#endif
		lua_settable(L, -3);

		if (bHaveG6ENV == false)
		{
			lua_setglobal(L, "G6ENV");
		}
		else
		{
			lua_pop(L, 1);
		}

		WriteG6lib(L);
	}

	bool IsLuaObjectExist(lua_State *L, UObject *Object)
	{
		if (::GetObjectMapping(L, Object))
		{
			lua_pop(L, 1);
			return true;
		}
		return false;
	}

	void SetLuaSourceRelativePath(const FString &RelativePath)
	{
		GLuaSrcRelativePath = RelativePath;
		GLuaSrcFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + GLuaSrcRelativePath + TEXT("/"));
	}

	FString GetLuaSourceRelativePath()
	{
		return GLuaSrcRelativePath;
	}

	FString GetLuaSourceFullPath()
	{
		return GLuaSrcFullPath;
	}

}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UnLuaFrameWorkModule, UnLuaFrameWork)
