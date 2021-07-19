#pragma once

#include "CoreUObject.h"

#include "Modules/ModuleManager.h"

class UnLuaFrameWorkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

namespace UnLua
{
	UNLUAFRAMEWORK_API bool IsLuaObjectExist(lua_State *L, UObject *Object);

	// relative path to FPaths::ProjectDir(), call it before initialization
	UNLUAFRAMEWORK_API void SetLuaSourceRelativePath(const FString &RelativePath);

	UNLUAFRAMEWORK_API FString GetLuaSourceRelativePath();

	UNLUAFRAMEWORK_API FString GetLuaSourceFullPath();

	UNLUAFRAMEWORK_API void RegisteG6Evn(lua_State* L);

	UNLUAFRAMEWORK_API bool GetRealFilePath(const FString& FilePath, FString& RealFilePath);
}