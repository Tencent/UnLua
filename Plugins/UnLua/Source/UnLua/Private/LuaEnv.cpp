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

#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "LuaEnv.h"
#include "Binding.h"
#include "LowLevel.h"
#include "Registries/ObjectRegistry.h"
#include "Registries/ClassRegistry.h"
#include "lstate.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "lualib.h"
#include "UELib.h"
#include "ObjectReferencer.h"
#include "UnLuaDelegates.h"
#include "UnLuaInterface.h"
#include "UnLuaLegacy.h"

namespace UnLua
{
    constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags::AsyncLoading | EInternalObjectFlags::Async;

    TMap<lua_State*, FLuaEnv*> FLuaEnv::AllEnvs;
    FLuaEnv::FOnCreated FLuaEnv::OnCreated;

    FLuaEnv::FLuaEnv()
    {
        RegisterDelegates();

        L = lua_newstate(GetLuaAllocator(), nullptr);
        AllEnvs.Add(L, this);

        luaL_openlibs(L);

        AddSearcher(LoadFromCustomLoader, 2);
        AddSearcher(LoadFromFileSystem, 3);
        AddSearcher(LoadFromBuiltinLibs, 4);

        UELib::Open(L);
        ObjectRegistry = MakeShared<FObjectRegistry>(this);
        ClassRegistry = MakeShared<FClassRegistry>(this);
        ClassRegistry->Register("UObject");
        ClassRegistry->Register("UClass");

        FunctionRegistry = MakeShared<FFunctionRegistry>(this);
        DelegateRegistry = MakeShared<FDelegateRegistry>(this);
        ContainerRegistry = MakeShared<FContainerRegistry>(this);
        EnumRegistry = MakeShared<FEnumRegistry>(this);
        DeadLoopCheck = MakeShared<FDeadLoopCheck>(this);

        AutoObjectReference.SetName("UnLua_AutoReference");
        ManualObjectReference.SetName("UnLua_ManualReference");

        lua_pushstring(L, "StructMap"); // create weak table 'StructMap'
        LowLevel::CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, "ArrayMap"); // create weak table 'ArrayMap'
        LowLevel::CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        // register global Lua functions
        lua_register(L, "GetUProperty", Global_GetUProperty);
        lua_register(L, "SetUProperty", Global_SetUProperty);
        lua_register(L, "LoadObject", Global_LoadObject);
        lua_register(L, "LoadClass", Global_LoadClass);
        lua_register(L, "NewObject", Global_NewObject);
        lua_register(L, "UEPrint", Global_Print);

        if (FUnLuaDelegates::ConfigureLuaGC.IsBound())
        {
            FUnLuaDelegates::ConfigureLuaGC.Execute(L);
        }
        else
        {
#if 504 == LUA_VERSION_NUM
            lua_gc(L, LUA_GCGEN);
#else
            // default Lua GC config in UnLua
            lua_gc(L, LUA_GCSETPAUSE, 100);
            lua_gc(L, LUA_GCSETSTEPMUL, 5000);
#endif
        }

        lua_register(L, "print", Global_Print);

        // add new package path
        const FString LuaSrcPath = GLuaSrcFullPath + TEXT("?.lua");
        AddPackagePath(L, TCHAR_TO_UTF8(*LuaSrcPath));

        FUnLuaDelegates::OnPreStaticallyExport.Broadcast();

        // register statically exported classes
        auto ExportedNonReflectedClasses = GetExportedNonReflectedClasses();
        for (const auto& Pair : ExportedNonReflectedClasses)
            Pair.Value->Register(L);

        // register statically exported global functions
        auto ExportedFunctions = GetExportedFunctions();
        for (const auto& Function : ExportedFunctions)
            Function->Register(L);

        // register statically exported enums
        auto ExportedEnums = GetExportedEnums();
        for (const auto& Enum : ExportedEnums)
            Enum->Register(L);

        DoString(R"(
            local ok, m = pcall(require, "UnLuaHotReload")
            if not ok then
                return
            end
            require = m.require
            UnLuaHotReload = m.reload
        )");

        OnCreated.Broadcast(*this);
        FUnLuaDelegates::OnLuaStateCreated.Broadcast(L);
    }

    FLuaEnv::~FLuaEnv()
    {
        lua_close(L);
        AllEnvs.Remove(L);

        if (Manager)
        {
            Manager->Cleanup();
            Manager->RemoveFromRoot();
        }

        AutoObjectReference.Clear();
        ManualObjectReference.Clear();

        UnRegisterDelegates();

        CandidateInputComponents.Empty();
        FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
    }

    FLuaEnv* FLuaEnv::FindEnv(const lua_State* L)
    {
        if (!L)
            return nullptr;

        const auto MainThread = G(L)->mainthread;
        return AllEnvs.FindRef(MainThread);
    }

    FLuaEnv& FLuaEnv::FindEnvChecked(const lua_State* L)
    {
        return *AllEnvs.FindChecked(G(L)->mainthread);
    }

    const FString& FLuaEnv::GetName()
    {
        return Name;
    }

    void FLuaEnv::SetName(FString InName)
    {
        Name = InName;
    }

    void FLuaEnv::NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index)
    {
        UObject* Object = (UObject*)ObjectBase;
        FunctionRegistry->NotifyUObjectDeleted(Object);
        if (Manager)
            Manager->NotifyUObjectDeleted(Object);
        ObjectRegistry->NotifyUObjectDeleted(Object);

        if (CandidateInputComponents.Num() <= 0)
            return;

        const int32 NumRemoved = CandidateInputComponents.Remove((UInputComponent*)Object);
        if (NumRemoved > 0 && CandidateInputComponents.Num() < 1)
            FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
    }

    void FLuaEnv::OnUObjectArrayShutdown()
    {
        GUObjectArray.RemoveUObjectDeleteListener(this);
        bObjectArrayListenerRegistered = false;
    }

    bool FLuaEnv::TryReplaceInputs(UObject* Object)
    {
        if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
            || !Object->IsA<UInputComponent>())
            return false;

        AActor* Actor = Cast<APlayerController>(Object->GetOuter());
        if (!Actor)
            Actor = Cast<APawn>(Object->GetOuter());

        if (!Actor || Actor->GetLocalRole() < ROLE_AutonomousProxy)
            return false;

        CandidateInputComponents.AddUnique((UInputComponent*)Object);
        if (OnWorldTickStartHandle.IsValid())
            FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
        OnWorldTickStartHandle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FLuaEnv::OnWorldTickStart);
        return true;
    }

    void FLuaEnv::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
    {
        if (!Manager)
            return;

        for (UInputComponent* InputComponent : CandidateInputComponents)
        {
            if (!InputComponent->IsRegistered() || InputComponent->IsPendingKill())
                continue;

            AActor* Actor = Cast<AActor>(InputComponent->GetOuter());
            Manager->ReplaceInputs(Actor, InputComponent); // try to replace/override input events
        }

        CandidateInputComponents.Empty();
        FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
    }

    bool FLuaEnv::TryBind(UObject* Object)
    {
        UClass* Class = Object->GetClass();
        if (Class->IsChildOf<UPackage>() || Class->IsChildOf<UClass>() || Class->HasAnyClassFlags(CLASS_NewerVersionExists))
        {
            // filter out UPackage and UClass and recompiled objects
            return false;
        }

        static UClass* InterfaceClass = UUnLuaInterface::StaticClass();
        const bool bImplUnluaInterface = Class->ImplementsInterface(InterfaceClass);
        
        if (!IsInGameThread() || Object->HasAnyInternalFlags(AsyncObjectFlags))
        {
            // avoid adding too many objects, affecting performance.
            if (bImplUnluaInterface || (!bImplUnluaInterface && GLuaDynamicBinding.IsValid(Class)))
            {
                // all bind operation should be in game thread, include dynamic bind
                FScopeLock Lock(&CandidatesLock);
                Candidates.AddUnique(Object);
                return false;
            }
        }

        if (!bImplUnluaInterface)
        {
            // dynamic binding
            if (!GLuaDynamicBinding.IsValid(Class))
                return false;

            return GetManager()->Bind(Object, *GLuaDynamicBinding.ModuleName, GLuaDynamicBinding.InitializerTableRef);
        }

        // filter some object in bp nest case
        // RF_WasLoaded & RF_NeedPostLoad?
        UObject* Outer = Object->GetOuter();
        if (Outer
            && Outer->GetFName().IsEqual("WidgetTree")
            && Object->HasAllFlags(RF_NeedInitialization | RF_NeedPostLoad | RF_NeedPostLoadSubobjects))
        {
            return false;
        }

        if (GWorld)
        {
            FString ObjectName;
            Object->GetFullName(GWorld, ObjectName);
            if (ObjectName.Contains(".WidgetArchetype:") || ObjectName.Contains(":WidgetTree."))
            {
                UE_LOG(LogUnLua, Warning, TEXT("Filter UObject of %s in WidgetArchetype"), *ObjectName);
                return false;
            }
        }

        UFunction* Func = Class->FindFunctionByName(FName("GetModuleName")); // find UFunction 'GetModuleName'. hard coded!!!
        if (!Func)
            return false;

        // native func may not be bind in level bp
        if (!Func->GetNativeFunc())
        {
            Func->Bind();
            if (!Func->GetNativeFunc())
            {
                UE_LOG(LogUnLua, Warning, TEXT("TryToBindLua: bind native function failed for GetModuleName in object %s"), *Object->GetName());
                return false;
            }
        }

        const bool bIsCDO = Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject);
        if ((bIsCDO && (Object->GetFlags() & RF_NeedInitialization)) || Class->GetName().Contains(TEXT("SKEL_")))
            return false;

        FString ModuleName;
        UObject* CDO = bIsCDO ? Object : Class->GetDefaultObject();
        CDO->ProcessEvent(Func, &ModuleName);
        if (ModuleName.IsEmpty())
            return false;

#if !UE_BUILD_SHIPPING
        if (GLuaDynamicBinding.IsValid(Class) && GLuaDynamicBinding.ModuleName != ModuleName)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Dynamic binding '%s' ignored as it conflicts static binding '%s'."), *GLuaDynamicBinding.ModuleName, *ModuleName);
        }
#endif

        return GetManager()->Bind(Object, *ModuleName, GLuaDynamicBinding.InitializerTableRef);
    }

    bool FLuaEnv::DoString(const FString& Chunk, const FString& ChunkName)
    {
        // TODO: env support
        // TODO: return value support
        const auto Guard = GetDeadLoopCheck()->MakeGuard();
        bool bOk = !luaL_dostring(L, TCHAR_TO_UTF8(*Chunk));
        if (bOk)
            return bOk;
        ReportLuaCallError(L);
        return false;
    }

    bool FLuaEnv::LoadBuffer(const char* Buffer, const size_t Size, const char* InName)
    {
        // TODO: env support
        // TODO: return value support

        // loads the buffer as a Lua chunk
        const int32 Code = luaL_loadbufferx(L, Buffer, Size, InName, nullptr);
        if (Code != LUA_OK)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Failed to call luaL_loadbufferx, error code: %d"), Code);
            ReportLuaCallError(L); // report pcall error
            lua_pushnil(L); /* error (message is on top of the stack) */
            lua_insert(L, -2); /* put before error message */
            return false;
        }

        return true;
    }

    void FLuaEnv::GC()
    {
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
    }

    void FLuaEnv::HotReload()
    {
        Call(L, "UnLuaHotReload");
    }

    int32 FLuaEnv::FindThread(const lua_State* Thread)
    {
        int32* ThreadRefPtr = ThreadToRef.Find(Thread);
        return ThreadRefPtr ? *ThreadRefPtr : LUA_REFNIL;
    }

    void FLuaEnv::ResumeThread(int32 ThreadRef)
    {
        lua_State** ThreadPtr = RefToThread.Find(ThreadRef);
        if (!ThreadPtr)
            return;

        lua_State* Thread = *ThreadPtr;
#if 504 == LUA_VERSION_NUM
        int NResults = 0;
        int32 Status = lua_resume(Thread, L, 0, &NResults);
#else
        int32 Status = lua_resume(Thread, L, 0);
#endif
        if (Status == LUA_YIELD)
            return;

        if (Status != LUA_OK)
        {
            const auto ErrMsg = lua_tostring(Thread, -1);
            UE_LOG(LogUnLua, Error, TEXT("%s"), UTF8_TO_TCHAR(ErrMsg));
        }

        ThreadToRef.Remove(Thread);
        RefToThread.Remove(ThreadRef);
        luaL_unref(L, LUA_REGISTRYINDEX, ThreadRef); // remove the reference if the coroutine finishes its execution
    }

    UUnLuaManager* FLuaEnv::GetManager()
    {
        if (!Manager)
        {
            Manager = NewObject<UUnLuaManager>();
            Manager->Env = this;
            Manager->AddToRoot();
        }
        return Manager;
    }
    
    void FLuaEnv::AddThread(lua_State* Thread, int32 ThreadRef)
    {
        ThreadToRef.Add(Thread, ThreadRef);
        RefToThread.Add(ThreadRef, Thread);
    }

    int32 FLuaEnv::FindOrAddThread(lua_State* Thread)
    {
        int32 ThreadRef = FindThread(Thread);
        if (ThreadRef == LUA_REFNIL)
        {
            int32 Value = lua_pushthread(Thread);
            if (Value == 1)
            {
                lua_pop(Thread, 1);
                return LUA_REFNIL;
            }

            ThreadRef = luaL_ref(Thread, LUA_REGISTRYINDEX);
            AddThread(Thread, ThreadRef);
        }
        return ThreadRef;
    }

    lua_Alloc FLuaEnv::GetLuaAllocator() const
    {
        return DefaultLuaAllocator;
    }

    void FLuaEnv::AddLoader(const FLuaFileLoader Loader)
    {
        CustomLoaders.Add(Loader);
    }

    void FLuaEnv::AddBuiltInLoader(const FString InName, const lua_CFunction Loader)
    {
        BuiltinLoaders.Add(InName, Loader);
    }

    int FLuaEnv::LoadFromBuiltinLibs(lua_State* L)
    {
        const FLuaEnv* Env = (FLuaEnv*)lua_touserdata(L, lua_upvalueindex(1));
        const FString Name = UTF8_TO_TCHAR(lua_tostring(L, 1));
        const auto Loader = Env->BuiltinLoaders.Find(Name);
        if (!Loader)
            return 0;
        lua_pushcfunction(L, *Loader);
        return 1;
    }

    int FLuaEnv::LoadFromCustomLoader(lua_State* L)
    {
        FLuaEnv* Env = (FLuaEnv*)lua_touserdata(L, lua_upvalueindex(1));
        if (FUnLuaDelegates::CustomLoadLuaFile.IsBound())
        {
            // legacy support
            const FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
            TArray<uint8> Data;
            FString ChunkName(TEXT("chunk"));
            if (FUnLuaDelegates::CustomLoadLuaFile.Execute(*Env, FileName, Data, ChunkName))
            {
                if (Env->LoadString(Data, ChunkName))
                    return 1;

                return luaL_error(L, "file loading from custom loader error");
            }
            return 0;
        }

        if (Env->CustomLoaders.Num() == 0)
            return 0;

        const FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));

        TArray<uint8> Data;
        FString ChunkName(TEXT("chunk"));
        for (auto Loader : Env->CustomLoaders)
        {
            if (!Loader.Execute(FileName, Data, ChunkName))
                continue;

            if (Env->LoadString(Data, ChunkName))
                break;

            return luaL_error(L, "file loading from custom loader error");
        }

        return 1;
    }

    int FLuaEnv::LoadFromFileSystem(lua_State* L)
    {
        FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
        FileName.ReplaceInline(TEXT("."), TEXT("/"));
        const auto RelativePath = FString::Printf(TEXT("%s.lua"), *FileName);
        const auto FullPath = GetFullPathFromRelativePath(RelativePath);
        TArray<uint8> Data;
        if (!FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
            return 0;

        const auto SkipLen = 3 < Data.Num() && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0; // skip UTF-8 BOM mark
        const auto ChunkName = TCHAR_TO_UTF8(*RelativePath);
        const auto Chunk = (const char*)(Data.GetData() + SkipLen);
        const auto ChunkSize = Data.Num() - SkipLen;
        if (!UnLua::LoadChunk(L, Chunk, ChunkSize, ChunkName))
            return luaL_error(L, "file loading from file system error");

        return 1;
    }

    void FLuaEnv::AddSearcher(lua_CFunction Searcher, int Index) const
    {
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "searchers");
        lua_remove(L, -2);
        if (!lua_istable(L, -1))
        {
            UE_LOG(LogUnLua, Warning, TEXT("Invalid package.serachers!"));
            return;
        }

        const uint32 Len = lua_rawlen(L, -1);
        Index = Index < 0 ? (int)(Len + Index + 2) : Index;
        for (int e = (int)Len + 1; e > Index; e--)
        {
            lua_rawgeti(L, -1, e - 1);
            lua_rawseti(L, -2, e);
        }

        lua_pushlightuserdata(L, (void*)this);
        lua_pushcclosure(L, Searcher, 1);
        lua_rawseti(L, -2, Index);
        lua_pop(L, 1);
    }

    void FLuaEnv::OnAsyncLoadingFlushUpdate()
    {
        TArray<FWeakObjectPtr> CandidatesTemp;
        TArray<int> CandidatesRemovedIndexes;

        TArray<UObject*> LocalCandidates;
        {
            {
                FScopeLock Lock(&CandidatesLock);
                CandidatesTemp.Append(Candidates);    
            }


            for (int32 i = CandidatesTemp.Num() - 1; i >= 0; --i)
            {
                FWeakObjectPtr ObjectPtr = CandidatesTemp[i];
                if (!ObjectPtr.IsValid())
                {
                    // discard invalid objects
                    CandidatesRemovedIndexes.Add(i);
                    continue;
                }

                UObject* Object = ObjectPtr.Get();
                if (Object->HasAnyFlags(RF_NeedPostLoad)
                    || Object->HasAnyInternalFlags(AsyncObjectFlags)
                    || Object->GetClass()->HasAnyInternalFlags(AsyncObjectFlags))
                {
                    // delay bind on next update 
                    continue;
                }

                LocalCandidates.Add(Object);
                CandidatesRemovedIndexes.Add(i);
            }
        }

        {
            FScopeLock Lock(&CandidatesLock);
            for(int32 j = 0; j < CandidatesRemovedIndexes.Num();++j)
            {
                Candidates.RemoveAt(CandidatesRemovedIndexes[j]);
            }
        }

        for (int32 i = 0; i < LocalCandidates.Num(); ++i)
        {
            UObject* Object = LocalCandidates[i];
            TryBind(Object);
        }
    }

    FORCEINLINE void FLuaEnv::RegisterDelegates()
    {
        OnAsyncLoadingFlushUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(this, &FLuaEnv::OnAsyncLoadingFlushUpdate);
        GUObjectArray.AddUObjectDeleteListener(this);
        bObjectArrayListenerRegistered = true;
    }

    FORCEINLINE void FLuaEnv::UnRegisterDelegates()
    {
        FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(OnAsyncLoadingFlushUpdateHandle);
        if (!bObjectArrayListenerRegistered)
            return;
        GUObjectArray.RemoveUObjectDeleteListener(this);
        bObjectArrayListenerRegistered = false;
    }

    void* FLuaEnv::DefaultLuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
    {
        if (nsize == 0)
        {
            UNLUA_STAT_MEMORY_FREE(ptr, Lua);
            FMemory::Free(ptr);
            return nullptr;
        }

        void* Buffer;
        if (!ptr)
        {
            Buffer = FMemory::Malloc(nsize);
            UNLUA_STAT_MEMORY_ALLOC(Buffer, Lua);
        }
        else
        {
            UNLUA_STAT_MEMORY_REALLOC(ptr, Buffer, Lua);
            Buffer = FMemory::Realloc(ptr, nsize);
        }
        return Buffer;
    }
}
