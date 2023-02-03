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

#include "Engine/EngineBaseTypes.h"
#include "Registries/ObjectRegistry.h"
#include "Registries/ClassRegistry.h"
#include "Registries/DelegateRegistry.h"
#include "Registries/FunctionRegistry.h"
#include "Registries/ContainerRegistry.h"
#include "Registries/PropertyRegistry.h"
#include "Registries/EnumRegistry.h"
#include "UnLuaManager.h"
#include "lua.hpp"
#include "ObjectReferencer.h"
#include "HAL/Platform.h"
#include "LuaDanglingCheck.h"
#include "LuaDeadLoopCheck.h"
#include "LuaModuleLocator.h"

namespace UnLua
{
    class UNLUA_API FLuaEnv
        : public FUObjectArray::FUObjectDeleteListener
    {
        friend FClassRegistry;
        friend FDelegateRegistry;
        friend FObjectRegistry;

    public:
        DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreated, FLuaEnv&);

        DECLARE_MULTICAST_DELEGATE_OneParam(FOnDestroyed, FLuaEnv&);

        DECLARE_DELEGATE_RetVal_FourParams(bool, FLuaFileLoader, const FLuaEnv& /* Env */, const FString& /* FilePath */, TArray<uint8>&/* Data */, FString&/* RealFilePath */);

        static FOnCreated OnCreated;

        static FOnDestroyed OnDestroyed;

        FLuaEnv();

        virtual ~FLuaEnv() override;

        static TMap<lua_State*, FLuaEnv*>& GetAll();

        static FLuaEnv* FindEnv(const lua_State* L);

        static FLuaEnv& FindEnvChecked(const lua_State* L);

        void Start(const TMap<FString, UObject*>& Args = {});

        void Start(const FString& StartupModuleName, const TMap<FString, UObject*>& Args);

        const FString& GetName();

        void SetName(FString InName);

        virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override;

        virtual void OnUObjectArrayShutdown() override;

        virtual bool TryBind(UObject* Object);

        virtual bool TryReplaceInputs(UObject* Object);

        bool DoString(const FString& Chunk, const FString& ChunkName = "chunk");

        virtual void GC();

        virtual void HotReload();

        FORCEINLINE lua_State* GetMainState() const { return L; }

        void AddThread(lua_State* Thread, int32 ThreadRef);

        int32 FindOrAddThread(lua_State* Thread);

        int32 FindThread(const lua_State* Thread);

        void ResumeThread(int32 ThreadRef);

        UUnLuaManager* GetManager();

        FORCEINLINE FClassRegistry* GetClassRegistry() const { return ClassRegistry; }

        FORCEINLINE FObjectRegistry* GetObjectRegistry() const { return ObjectRegistry; }

        FORCEINLINE FDelegateRegistry* GetDelegateRegistry() const { return DelegateRegistry; }

        FORCEINLINE FFunctionRegistry* GetFunctionRegistry() const { return FunctionRegistry; }

        FORCEINLINE FContainerRegistry* GetContainerRegistry() const { return ContainerRegistry; }

        FORCEINLINE FEnumRegistry* GetEnumRegistry() const { return EnumRegistry; }

        FORCEINLINE FPropertyRegistry* GetPropertyRegistry() const { return PropertyRegistry; }

        FORCEINLINE FDanglingCheck* GetDanglingCheck() const { return DanglingCheck; }

        FORCEINLINE FDeadLoopCheck* GetDeadLoopCheck() const { return DeadLoopCheck; }

        void AddLoader(const FLuaFileLoader Loader);

        void AddBuiltInLoader(const FString InName, lua_CFunction Loader);

        void AddManualObjectReference(UObject* Object);

        void RemoveManualObjectReference(UObject* Object);

    protected:
        lua_State* L;

        static int LoadFromBuiltinLibs(lua_State* L);

        static int LoadFromCustomLoader(lua_State* L);

        static int LoadFromFileSystem(lua_State* L);

        static void* DefaultLuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize);

        virtual lua_Alloc GetLuaAllocator() const;

        bool LoadString(lua_State* InL, const TArray<uint8>& Chunk, const FString& ChunkName = "chunk")
        {
            const char* Bytes = (char*)Chunk.GetData();
            return LoadBuffer(InL, Bytes, Chunk.Num(), TCHAR_TO_UTF8(*ChunkName));
        }

        bool LoadString(lua_State* InL, const FString& Chunk, const FString& ChunkName = "chunk")
        {
            const FTCHARToUTF8 Bytes(*Chunk);
            return LoadBuffer(InL, Bytes.Get(), Bytes.Length(), TCHAR_TO_UTF8(*ChunkName));
        }

    private:
        void AddSearcher(lua_CFunction Searcher, int Index) const;

        bool LoadBuffer(lua_State* InL, const char* Buffer, const size_t Size, const char* InName);

        void OnAsyncLoadingFlushUpdate();

        void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

        void RegisterDelegates();

        void UnRegisterDelegates();

        static TMap<lua_State*, FLuaEnv*> AllEnvs;
        TMap<FString, lua_CFunction> BuiltinLoaders;
        TArray<FLuaFileLoader> CustomLoaders;
        TArray<FWeakObjectPtr> Candidates; // binding candidates during async loading
        ULuaModuleLocator* ModuleLocator;
        FCriticalSection CandidatesLock;
        FObjectReferencer AutoObjectReference;
        FObjectReferencer ManualObjectReference;
        UUnLuaManager* Manager = nullptr;
        FClassRegistry* ClassRegistry;
        FObjectRegistry* ObjectRegistry;
        FDelegateRegistry* DelegateRegistry;
        FFunctionRegistry* FunctionRegistry;
        FContainerRegistry* ContainerRegistry;
        FPropertyRegistry* PropertyRegistry;
        FEnumRegistry* EnumRegistry;
        FDanglingCheck* DanglingCheck;
        FDeadLoopCheck* DeadLoopCheck;
        TMap<lua_State*, int32> ThreadToRef;
        TMap<int32, lua_State*> RefToThread;
        FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
        TArray<UInputComponent*> CandidateInputComponents;
        FDelegateHandle OnWorldTickStartHandle;
        FString Name = TEXT("Env_0");
        bool bObjectArrayListenerRegistered;
        bool bStarted;
    };
}
