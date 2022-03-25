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

#include "LuaEnv.h"
#include "UObject/UObjectArray.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericApplication.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UnLuaBase.h"
#include "UnLuaManager.h"
#include "HAL/Platform.h"

class FLuaContext : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
public:
    UNLUA_API static FLuaContext* Create();

    void RegisterDelegates();

    void CreateState();
    void SetEnable(bool InEnable);
    bool IsEnable() const;

    TSharedPtr<UnLua::ITypeInterface> FindTypeInterface(FName Name);

    bool TryToBindLua(UObject *Object);

    void AddLibraryName(const TCHAR *LibraryName) { LibraryNames.Add(LibraryName); }

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 23)
    void OnWorldTickStart(UWorld *World, ELevelTick TickType, float DeltaTime);
#else
    void OnWorldTickStart(ELevelTick TickType, float DeltaTime);
#endif
    void OnWorldCleanup(UWorld *World, bool bSessionEnded, bool bCleanupResources);
    void OnPostEngineInit();
    void OnPreExit();
    void OnCrash();
    void PostLoadMapWithWorld(UWorld *World);
    void OnPostGarbageCollect();

#if WITH_EDITOR
    void PreBeginPIE(bool bIsSimulating);
    void PostPIEStarted(bool bIsSimulating);
    void PrePIEEnded(bool bIsSimulating);
#endif

    void AddThread(lua_State *Thread, int32 ThreadRef);
    void ResumeThread(int32 ThreadRef);
    void CleanupThreads();
    int32 FindThread(lua_State *Thread);

    FORCEINLINE operator lua_State*() const { return Env != nullptr ? Env->GetMainState() : nullptr; }

    // interfaces of FUObjectArray::FUObjectCreateListener and FUObjectArray::FUObjectDeleteListener
    virtual void NotifyUObjectCreated(const class UObjectBase *InObject, int32 Index) override;
    virtual void NotifyUObjectDeleted(const class UObjectBase *InObject, int32 Index) override;
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
	virtual void OnUObjectArrayShutdown() override;
#endif

    UUnLuaManager* GetUnLuaManager();

private:
    FLuaContext();
    ~FLuaContext();

    void Initialize();
    void Cleanup(bool bFullCleanup = false, UWorld *World = nullptr);

    bool OnGameViewportInputKey(FKey InKey, FModifierKeysState ModifierKeyState, EInputEvent EventType);

    TUniquePtr<UnLua::FLuaEnv> Env;

    FDelegateHandle OnActorSpawnedHandle;
    FDelegateHandle OnWorldTickStartHandle;
    FDelegateHandle OnPostGarbageCollectHandle;

    TArray<FString> LibraryNames;       // metatables for classes/enums

    //!!!Fix!!!
    //thread need refine
    TMap<lua_State*, int32> ThreadToRef;                                // coroutine -> ref
    TMap<int32, lua_State*> RefToThread;                                // ref -> coroutine
    TMap<UObjectBase*, FString> UObjPtr2Name;                           // UObject pointer -> Name for debug purpose
    FCriticalSection Async2MainCS;                                      // async loading thread and main thread sync lock

    TArray<class UInputComponent*> CandidateInputComponents;
    TArray<UGameInstance*> GameInstances;

    bool bEnable;
};

extern class FLuaContext *GLuaCxt;
