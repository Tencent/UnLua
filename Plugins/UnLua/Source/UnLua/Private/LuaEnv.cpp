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
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "UELib.h"
#include "ObjectReferencer.h"
#include "UnLuaDelegates.h"
#include "UnLuaInterface.h"
#include "UnLuaLegacy.h"
#include "UnLuaLib.h"
#include "UnLuaSettings.h"
#include "lstate.h"

namespace UnLua
{
    constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags::AsyncLoading | EInternalObjectFlags::Async;

    TMap<lua_State*, FLuaEnv*> FLuaEnv::AllEnvs;
    FLuaEnv::FOnCreated FLuaEnv::OnCreated;
    FLuaEnv::FOnDestroyed FLuaEnv::OnDestroyed;

#if ENABLE_UNREAL_INSIGHTS && CPUPROFILERTRACE_ENABLED
    void Hook(lua_State* L, lua_Debug* ar)
    {
        static TSet<FName> IgnoreNames{FName("Class"), FName("index"), FName("newindex")};

        lua_getinfo(L, "nSl", ar);

        if (ar->what == FName("Lua"))
        {
            if (IgnoreNames.Contains(ar->name))
            {
                return;
            }

            const auto EventName = FString::Printf(TEXT(
                "%s [%s:%d]"),
                                                   *FString(ar->name ? ar->name : "N/A"),
                                                   *FPaths::GetBaseFilename(FString(ar->source)),
                                                   ar->linedefined);

            if (ar->event == 0)
            {
                FCpuProfilerTrace::OutputBeginDynamicEvent(*EventName);
            }
            else
            {
                FCpuProfilerTrace::OutputEndEvent();
            }
        }
    }
#endif

    FLuaEnv::FLuaEnv()
        : bStarted(false)
    {
        const auto Settings = GetDefault<UUnLuaSettings>();
        ModuleLocator = Settings->ModuleLocatorClass.GetDefaultObject();
        ensureMsgf(ModuleLocator, TEXT("Invalid lua module locator, lua binding will not work properly. please check unlua runtime settings."));

        RegisterDelegates();

#if PLATFORM_WINDOWS
        // 防止类似AppleProResMedia插件忘了恢复Dll查找目录
        // https://github.com/Tencent/UnLua/issues/534
        const auto Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Binaries/Win64"));
        FPlatformProcess::PushDllDirectory(*Dir);
        L = lua_newstate(GetLuaAllocator(), nullptr);
        FPlatformProcess::PopDllDirectory(*Dir);
#else
        L = lua_newstate(GetLuaAllocator(), nullptr);
#endif

        AllEnvs.Add(L, this);

        luaL_openlibs(L);

        AddSearcher(LoadFromCustomLoader, 2);
        AddSearcher(LoadFromFileSystem, 3);
        AddSearcher(LoadFromBuiltinLibs, 4);

        UELib::Open(L);

        ObjectRegistry = new FObjectRegistry(this);
        ClassRegistry = new FClassRegistry(this);
        ClassRegistry->Initialize();

        FunctionRegistry = new FFunctionRegistry(this);
        DelegateRegistry = new FDelegateRegistry(this);
        ContainerRegistry = new FContainerRegistry(this);
        PropertyRegistry = new FPropertyRegistry(this);
        EnumRegistry = new FEnumRegistry(this);
        EnumRegistry->Initialize();

        DanglingCheck = new FDanglingCheck(this);
        DeadLoopCheck = new FDeadLoopCheck(this);

        AutoObjectReference.SetName("UnLua_AutoReference");
        ManualObjectReference.SetName("UnLua_ManualReference");

        lua_pushstring(L, "StructMap"); // create weak table 'StructMap'
        LowLevel::CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        lua_pushstring(L, "ArrayMap"); // create weak table 'ArrayMap'
        LowLevel::CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        if (FUnLuaDelegates::ConfigureLuaGC.IsBound())
        {
            FUnLuaDelegates::ConfigureLuaGC.Execute(L);
        }
        else
        {
#if 504 == LUA_VERSION_NUM
            lua_gc(L, LUA_GCGEN, 0, 0);
#else
            // default Lua GC config in UnLua
            lua_gc(L, LUA_GCSETPAUSE, 100);
            lua_gc(L, LUA_GCSETSTEPMUL, 5000);
#endif
        }

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

        UnLuaLib::Open(L);

        OnCreated.Broadcast(*this);
        FUnLuaDelegates::OnLuaStateCreated.Broadcast(L);

#if ENABLE_UNREAL_INSIGHTS && CPUPROFILERTRACE_ENABLED
        if (FDeadLoopCheck::Timeout)
            UE_LOG(LogUnLua, Warning, TEXT("Profiling will not working when DeadLoopCheck enabled."))
        else
            lua_sethook(L, Hook, LUA_MASKCALL | LUA_MASKRET, 0);
#endif
    }

    FLuaEnv::~FLuaEnv()
    {
        OnDestroyed.Broadcast(*this);
        lua_close(L);
        AllEnvs.Remove(L);

        delete ClassRegistry;
        delete ObjectRegistry;
        delete DelegateRegistry;
        delete FunctionRegistry;
        delete ContainerRegistry;
        delete EnumRegistry;
        delete PropertyRegistry;
        delete DanglingCheck;
        delete DeadLoopCheck;

        if (!IsEngineExitRequested() && Manager)
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

    TMap<lua_State*, FLuaEnv*>& FLuaEnv::GetAll()
    {
        return AllEnvs;
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

    void FLuaEnv::Start(const TMap<FString, UObject*>& Args)
    {
        const auto& Setting = *GetDefault<UUnLuaSettings>();
        Start(Setting.StartupModuleName, Args);
    }

    void FLuaEnv::Start(const FString& StartupModuleName, const TMap<FString, UObject*>& Args)
    {
        if (bStarted)
            return;

        if (StartupModuleName.IsEmpty())
        {
            bStarted = true;
            return;
        }

        const auto Guard = GetDeadLoopCheck()->MakeGuard();
        lua_pushcfunction(L, ReportLuaCallError);
        lua_getglobal(L, "require");
        lua_pushstring(L, TCHAR_TO_UTF8(*StartupModuleName));
        lua_createtable(L, 0, Args.Num());
        for (auto Pair : Args)
        {
            PushUObject(L, Pair.Value);
            lua_setfield(L, -2, TCHAR_TO_UTF8(*Pair.Key));
        }
        lua_pcall(L, 2, LUA_MULTRET, -4);
        bStarted = true;
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
        PropertyRegistry->NotifyUObjectDeleted(Object);
        FunctionRegistry->NotifyUObjectDeleted(Object);
        if (Manager)
            Manager->NotifyUObjectDeleted(Object);
        ObjectRegistry->NotifyUObjectDeleted(Object);
        ClassRegistry->NotifyUObjectDeleted(Object);
        EnumRegistry->NotifyUObjectDeleted(Object);

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

        if (!Actor)
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
            if (!InputComponent->IsRegistered())
                continue;

#if ENGINE_MAJOR_VERSION >=5
            if (!IsValid(InputComponent))
                continue;
#else
            if (InputComponent->IsPendingKill())
                continue;
#endif

            AActor* Actor = Cast<AActor>(InputComponent->GetOuter());
            Manager->ReplaceInputs(Actor, InputComponent); // try to replace/override input events
        }

        CandidateInputComponents.Empty();
        FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartHandle);
    }

    bool FLuaEnv::TryBind(UObject* Object)
    {
        const auto Class = Object->IsA<UClass>() ? static_cast<UClass*>(Object) : Object->GetClass();
        if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
        {
            // filter out recompiled objects
            return false;
        }

        static UClass* InterfaceClass = UUnLuaInterface::StaticClass();
        const bool bImplUnluaInterface = Class->ImplementsInterface(InterfaceClass);

        if (IsInAsyncLoadingThread())
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

        if (Class->GetName().Contains(TEXT("SKEL_")))
            return false;

        if (!ensureMsgf(ModuleLocator, TEXT("Invalid lua module locator, lua binding will not work properly. please check unlua runtime settings.")))
            return false;

        const auto ModuleName = ModuleLocator->Locate(Object);
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
        const FTCHARToUTF8 ChunkUTF8(*Chunk);
        const FTCHARToUTF8 ChunkNameUTF8(*ChunkName);
        const auto Guard = GetDeadLoopCheck()->MakeGuard();
        const auto DanglingGuard = GetDanglingCheck()->MakeGuard();
        lua_pushcfunction(L, ReportLuaCallError);
        const auto MsgHandlerIdx = lua_gettop(L);
        if (!LoadBuffer(L, ChunkUTF8.Get(), ChunkUTF8.Length(), ChunkNameUTF8.Get()))
        {
            lua_pop(L, 1);
            return false;
        }

        const auto Result = lua_pcall(L, 0, LUA_MULTRET, MsgHandlerIdx);
        if (Result == LUA_OK)
        {
            lua_remove(L, MsgHandlerIdx);
            return true;
        }
        lua_pop(L, lua_gettop(L) - MsgHandlerIdx + 1);
        return false;
    }

    bool FLuaEnv::LoadBuffer(lua_State* InL, const char* Buffer, const size_t Size, const char* InName)
    {
        // TODO: env support
        // TODO: return value support

#if UNLUA_LEGACY_ALLOW_BOM || WITH_EDITOR
        if (Size > 3 && Buffer[0] == static_cast<char>(0xEF) && Buffer[1] == static_cast<char>(0xBB) && Buffer[2] == static_cast<char>(0xBF))
        {
#if !UNLUA_LEGACY_ALLOW_BOM
            UE_LOG(LogUnLua, Warning, TEXT("Lua chunk with utf-8 BOM:%s"), UTF8_TO_TCHAR(InName));
#endif
            return LoadBuffer(InL, Buffer + 3, Size - 3, InName);
        }
#endif

        // loads the buffer as a Lua chunk
        const int32 Code = luaL_loadbufferx(InL, Buffer, Size, InName, nullptr);
        if (Code != LUA_OK)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Failed to call luaL_loadbufferx, error code: %d"), Code);
            ReportLuaCallError(InL); // report pcall error
            lua_pushnil(InL); /* error (message is on top of the stack) */
            lua_insert(InL, -2); /* put before error message */
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
        DoString("UnLua.HotReload()");
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

    void FLuaEnv::AddManualObjectReference(UObject* Object)
    {
        ManualObjectReference.Add(Object);
    }

    void FLuaEnv::RemoveManualObjectReference(UObject* Object)
    {
        ManualObjectReference.Remove(Object);
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
        auto& Env = *(FLuaEnv*)lua_touserdata(L, lua_upvalueindex(1));
        if (FUnLuaDelegates::CustomLoadLuaFile.IsBound())
        {
            // legacy support
            const FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
            TArray<uint8> Data;
            FString ChunkName(TEXT("chunk"));
            if (FUnLuaDelegates::CustomLoadLuaFile.Execute(Env, FileName, Data, ChunkName))
            {
                if (Env.LoadString(L, Data, ChunkName))
                    return 1;

                return luaL_error(L, "file loading from custom loader error");
            }
            return 0;
        }

        if (Env.CustomLoaders.Num() == 0)
            return 0;

        const FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));

        TArray<uint8> Data;
        FString ChunkName(TEXT("chunk"));
        for (auto& Loader : Env.CustomLoaders)
        {
            if (!Loader.Execute(Env, FileName, Data, ChunkName))
                continue;

            if (Env.LoadString(L, Data, ChunkName))
                break;

            return luaL_error(L, "file loading from custom loader error");
        }

        return 1;
    }

    int FLuaEnv::LoadFromFileSystem(lua_State* L)
    {
        FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
        FileName.ReplaceInline(TEXT("."), TEXT("/"));

        auto& Env = *(FLuaEnv*)lua_touserdata(L, lua_upvalueindex(1));
        TArray<uint8> Data;
        FString FullPath;

        auto LoadIt = [&]
        {
            if (Env.LoadString(L, Data, FullPath))
                return 1;
            const auto Msg = FString::Printf(TEXT("file loading from file system error.\nfull path:%s"), *FullPath);
            return luaL_error(L, TCHAR_TO_UTF8(*Msg));
        };

        const auto PackagePath = UnLuaLib::GetPackagePath(L);
        if (PackagePath.IsEmpty())
            return 0;

        TArray<FString> Patterns;
        if (PackagePath.ParseIntoArray(Patterns, TEXT(";"), false) == 0)
            return 0;

        // 优先加载下载目录下的单文件
        for (auto& Pattern : Patterns)
        {
            Pattern.ReplaceInline(TEXT("?"), *FileName);
            const auto PathWithPersistentDir = FPaths::Combine(FPaths::ProjectPersistentDownloadDir(), Pattern);
            FullPath = FPaths::ConvertRelativePathToFull(PathWithPersistentDir);
            if (FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
                return LoadIt();
        }

        // 其次是打包目录下的文件
        for (auto& Pattern : Patterns)
        {
            const auto PathWithProjectDir = FPaths::Combine(FPaths::ProjectDir(), Pattern);
            FullPath = FPaths::ConvertRelativePathToFull(PathWithProjectDir);
            if (FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
                return LoadIt();
        }

        return 0;
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
            for (int32 j = 0; j < CandidatesRemovedIndexes.Num(); ++j)
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
