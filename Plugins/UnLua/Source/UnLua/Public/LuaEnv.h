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

#include "Registries/ObjectRegistry.h"
#include "Registries/ClassRegistry.h"
#include "Registries/DelegateRegistry.h"
#include "Registries/FunctionRegistry.h"
#include "Registries/ContainerRegistry.h"
#include "Registries/EnumRegistry.h"
#include "UnLuaManager.h"
#include "lua.h"
#include "LuaValue.h"
#include "UnLuaLegacy.h"
#include "ObjectReferencer.h"
#include "HAL/Platform.h"

namespace UnLua
{
    class UNLUA_API FLuaEnv
        : public FUObjectArray::FUObjectDeleteListener
    {
        friend FClassRegistry;
        friend FDelegateRegistry;

    public:
        DECLARE_DELEGATE_RetVal_ThreeParams(bool, FLuaFileLoader, const FString& /* FilePath */, TArray<uint8>&/* Data */, FString&/* RealFilePath */);

        FLuaEnv();

        virtual ~FLuaEnv() override;

        static FLuaEnv* FindEnv(const lua_State* L);

        static FLuaEnv& FindEnvChecked(const lua_State* L);

        const FString& GetName();

        void SetName(FString InName);

        virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override;

        virtual void OnUObjectArrayShutdown() override;

        virtual bool TryBind(UObject* Object);

        virtual bool TryReplaceInputs(UObject* Object);

        bool DoString(const FString& Chunk, const FString& ChunkName = "chunk");

        bool LoadString(const TArray<uint8>& Chunk, const FString& ChunkName = "chunk")
        {
            const char* Bytes = (char*)Chunk.GetData();
            return LoadBuffer(Bytes, Chunk.Num(), TCHAR_TO_UTF8(*ChunkName));
        }

        bool LoadString(const FString& Chunk, const FString& ChunkName = "chunk")
        {
            const FTCHARToUTF8 Bytes(*Chunk);
            return LoadBuffer(Bytes.Get(), Bytes.Length(), TCHAR_TO_UTF8(*ChunkName));
        }

        // TODO: refactor this
        template <typename ... T>
        FLuaRetValues CallTableFunc(const FString& TableName, const FString& FuncName, T&&... Args)
        {
            return UnLua::CallTableFunc(L, TCHAR_TO_UTF8(*TableName), TCHAR_TO_UTF8(*FuncName), Forward<T>(Args)...);
        }

        virtual void GC();

        virtual void HotReload();

        lua_State* GetMainState() const { return L; }

        void AddThread(lua_State* Thread, int32 ThreadRef);

        int32 FindOrAddThread(lua_State* Thread);

        int32 FindThread(const lua_State* Thread);

        void ResumeThread(int32 ThreadRef);

        UUnLuaManager* GetManager() const { return Manager; }

        TSharedPtr<FClassRegistry> GetClassRegistry() const { return ClassRegistry; }

        TSharedPtr<FObjectRegistry> GetObjectRegistry() const { return ObjectRegistry; }

        TSharedPtr<FDelegateRegistry> GetDelegateRegistry() const { return DelegateRegistry; }

        TSharedPtr<FFunctionRegistry> GetFunctionRegistry() const { return FunctionRegistry; }

        TSharedPtr<FContainerRegistry> GetContainerRegistry() const { return ContainerRegistry; }

        TSharedPtr<FEnumRegistry> GetEnumRegistry() const { return EnumRegistry; }

        void AddLoader(const FLuaFileLoader Loader);

        void AddBuiltInLoader(const FString InName, lua_CFunction Loader);

    protected:
        lua_State* L;

        static int LoadFromBuiltinLibs(lua_State* L);

        static int LoadFromCustomLoader(lua_State* L);

        static int LoadFromFileSystem(lua_State* L);

        static void* DefaultLuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize);

        virtual lua_Alloc GetLuaAllocator() const;

    private:
        void AddSearcher(lua_CFunction Searcher, int Index) const;

        bool LoadBuffer(const char* Buffer, const size_t Size, const char* InName);

        void OnAsyncLoadingFlushUpdate();

        void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

        void RegisterDelegates();

        void UnRegisterDelegates();

        static TMap<lua_State*, FLuaEnv*> AllEnvs;
        TMap<FString, lua_CFunction> BuiltinLoaders;
        TArray<FLuaFileLoader> CustomLoaders;
        TArray<FWeakObjectPtr> Candidates; // binding candidates during async loading
        FCriticalSection CandidatesLock;
        FObjectReferencer AutoObjectReference;
        FObjectReferencer ManualObjectReference;
        UUnLuaManager* Manager;
        TSharedPtr<FClassRegistry> ClassRegistry;
        TSharedPtr<FObjectRegistry> ObjectRegistry;
        TSharedPtr<FDelegateRegistry> DelegateRegistry;
        TSharedPtr<FFunctionRegistry> FunctionRegistry;
        TSharedPtr<FContainerRegistry> ContainerRegistry;
        TSharedPtr<FEnumRegistry> EnumRegistry;
        TMap<lua_State*, int32> ThreadToRef;
        TMap<int32, lua_State*> RefToThread;
        FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
        TArray<UInputComponent*> CandidateInputComponents;
        FDelegateHandle OnWorldTickStartHandle;
        FString Name = TEXT("Env_0");
        bool bObjectArrayListenerRegistered;
    };
}
