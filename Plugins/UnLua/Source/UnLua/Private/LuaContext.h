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

#include "UObject/UObjectArray.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericApplication.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UnLuaBase.h"

class FLuaContext : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
public:
    UNLUA_API static FLuaContext* Create();

    void RegisterDelegates();

    void CreateState();
    void SetEnable(bool InEnable);
    bool IsEnable() const;

    bool ExportFunction(UnLua::IExportedFunction *Function);
    bool ExportEnum(UnLua::IExportedEnum *Enum);
    bool ExportClass(UnLua::IExportedClass *Class);
    UnLua::IExportedClass* FindExportedClass(FName Name);
    UnLua::IExportedClass* FindExportedReflectedClass(FName Name);
    UnLua::IExportedClass* FindExportedNonReflectedClass(FName Name);

    bool AddTypeInterface(FName Name, TSharedPtr<UnLua::ITypeInterface> TypeInterface);
    TSharedPtr<UnLua::ITypeInterface> FindTypeInterface(FName Name);

    bool TryToBindLua(UObjectBaseUtility *Object);

    void AddLibraryName(const TCHAR *LibraryName) { LibraryNames.Add(LibraryName); }
    void AddModuleName(const TCHAR *ModuleName) { ModuleNames.AddUnique(ModuleName); }

#if ENGINE_MINOR_VERSION > 23
    void OnWorldTickStart(UWorld *World, ELevelTick TickType, float DeltaTime);
#else
    void OnWorldTickStart(ELevelTick TickType, float DeltaTime);
#endif
    void OnWorldCleanup(UWorld *World, bool bSessionEnded, bool bCleanupResources);
    void OnBeginFrame();
    void OnPostEngineInit();
    void OnPreExit();
    void OnAsyncLoadingFlushUpdate();
    void OnCrash();
    void PostLoadMapWithWorld(UWorld *World);
    void OnPostGarbageCollect();
    void OnDelayBindObject(UObject* Object);

#if WITH_EDITOR
    void PreBeginPIE(bool bIsSimulating);
    void PostPIEStarted(bool bIsSimulating);
    void PrePIEEnded(bool bIsSimulating);
#endif

    const TMap<FName, UnLua::IExportedClass*>& GetExportedReflectedClasses() const { return ExportedReflectedClasses; }
    const TMap<FName, UnLua::IExportedClass*>& GetExportedNonReflectedClasses() const { return ExportedNonReflectedClasses; }
    const TArray<UnLua::IExportedEnum*>& GetExportedEnums() const { return ExportedEnums; }
    const TArray<UnLua::IExportedFunction*>& GetExportedFunctions() const { return ExportedFunctions; }


    void AddThread(lua_State *Thread, int32 ThreadRef);
    void ResumeThread(int32 ThreadRef);
    void CleanupThreads();
    int32 FindThread(lua_State *Thread);

    FORCEINLINE class UUnLuaManager* GetManager() const { return Manager; }

    FORCEINLINE operator lua_State*() const { return L; }

    // interfaces of FUObjectArray::FUObjectCreateListener and FUObjectArray::FUObjectDeleteListener
    virtual void NotifyUObjectCreated(const class UObjectBase *InObject, int32 Index) override;
    virtual void NotifyUObjectDeleted(const class UObjectBase *InObject, int32 Index) override;
#if ENGINE_MINOR_VERSION > 22
	virtual void OnUObjectArrayShutdown() override;
#endif

	bool IsUObjectValid(UObjectBase* UObjPtr);

    UUnLuaManager* GetUnLuaManager();

private:
    FLuaContext();
    ~FLuaContext();

    // allocator used for 'lua_newstate'
    static void* LuaAllocator(void *ud, void *ptr, size_t osize, size_t nsize);

    void Initialize();
    void Cleanup(bool bFullCleanup = false, UWorld *World = nullptr);

    bool OnGameViewportInputKey(FKey InKey, FModifierKeysState ModifierKeyState, EInputEvent EventType);


    lua_State *L;

    UUnLuaManager *Manager;

    FDelegateHandle OnActorSpawnedHandle;
    FDelegateHandle OnWorldTickStartHandle;
    FDelegateHandle OnPostGarbageCollectHandle;

    TArray<FString> LibraryNames;       // metatables for classes/enums
    TArray<FString> ModuleNames;        // required Lua modules

    TArray<UObject*> Candidates;        // binding candidates during async loading
    TArray<UObject*> PostLoadObjects;        // binding object which need post load

    TArray<UnLua::IExportedFunction*> ExportedFunctions;                // statically exported global functions
    TArray<UnLua::IExportedEnum*> ExportedEnums;                        // statically exported enums
    TMap<FName, UnLua::IExportedClass*> ExportedReflectedClasses;       // statically exported reflected classes
    TMap<FName, UnLua::IExportedClass*> ExportedNonReflectedClasses;    // statically exported non-reflected classes

    TMap<FName, TSharedPtr<UnLua::ITypeInterface>> TypeInterfaces;      // registered type interfaces

    //!!!Fix!!!
    //thread need refine
    TMap<lua_State*, int32> ThreadToRef;                                // coroutine -> ref
    TMap<int32, lua_State*> RefToThread;                                // ref -> coroutine
	TMap<UObjectBase *, int32> UObjPtr2Idx;                             // UObject pointer -> index in GUObjectArray
    TMap<UObjectBase*, FString> UObjPtr2Name;                           // UObject pointer -> Name for debug purpose
    FCriticalSection Async2MainCS;                                      // async loading thread and main thread sync lock

#if WITH_EDITOR
    void *LuaHandle;
#endif

    TArray<class UInputComponent*> CandidateInputComponents;
    TArray<UGameInstance*> GameInstances;

    bool bEnable;
};

extern class FLuaContext *GLuaCxt;
