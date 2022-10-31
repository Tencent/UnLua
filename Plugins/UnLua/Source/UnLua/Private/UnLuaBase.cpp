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

#include "LowLevel.h"
#include "LuaCore.h"
#include "UnLuaDelegates.h"
#include "ObjectReferencer.h"
#include "UnLuaModule.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"

DEFINE_LOG_CATEGORY(LogUnLua);
DEFINE_LOG_CATEGORY(UnLuaDelegate);

namespace UnLua
{
    bool IsUObjectValid(UObjectBase* ObjPtr)
    {
        if (!ObjPtr || ObjPtr == LowLevel::ReleasedPtr)
            return false;
        return (ObjPtr->GetFlags() & (RF_BeginDestroyed | RF_FinishDestroyed)) == 0 && ObjPtr->IsValidLowLevelFast();
    }

    lua_State* CreateState()
    {
        IUnLuaModule::Get().SetActive(true);
        return GetState();
    }

    lua_State* GetState()
    {
        const auto Env = IUnLuaModule::Get().GetEnv();
        return Env ? Env->GetMainState() : nullptr;
    }

    bool Startup()
    {
        IUnLuaModule::Get().SetActive(true);
        return true;
    }

    void Shutdown()
    {
        IUnLuaModule::Get().SetActive(false);
    }

    bool IsEnabled()
    {
        return IUnLuaModule::Get().IsActive();
    }

    /**
     * Load a Lua file without running it
     */
    bool LoadFile(lua_State *L, const FString &RelativeFilePath, const char *Mode, int32 Env)
    {
        FString FullFilePath = GetFullPathFromRelativePath(RelativeFilePath);
        if (FullFilePath.IsEmpty())
        {
            UE_LOG(LogUnLua, Warning, TEXT("the lua file try to load does not exist! : %s"), *RelativeFilePath);
            return false;
        }

        if (FullFilePath != GLuaSrcFullPath + RelativeFilePath)
        {
            UE_LOG(LogUnLua, Log, TEXT("Load lua file from DownloadDir : %s"), *FullFilePath);
        }

        TArray<uint8> Data;
        bool bSuccess = FFileHelper::LoadFileToArray(Data, *FullFilePath, 0);
        if (!bSuccess)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to load lua file!"), ANSI_TO_TCHAR(__FUNCTION__));
            return false;
        }

        int32 SkipLen = (3 < Data.Num()) && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0;        // skip UTF-8 BOM mark
        return LoadChunk(L, (const char*)(Data.GetData() + SkipLen), Data.Num() - SkipLen, TCHAR_TO_UTF8(*RelativeFilePath), Mode, Env);    // loads the buffer as a Lua chunk
    }

    /**
     * Load a Lua chunk without running it
     */
    bool LoadChunk(lua_State *L, const char *Chunk, int32 ChunkSize, const char *ChunkName, const char *Mode, int32 Env)
    {
        int32 Code = luaL_loadbufferx(L, Chunk, ChunkSize, ChunkName, Mode);        // loads the buffer as a Lua chunk
        if (Code != LUA_OK)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Failed to call luaL_loadbufferx, error code: %d"), Code);
            ReportLuaCallError(L);                          // report pcall error
        }

        if (Code == LUA_OK)
        {
            if (Env != 0)
            {
                /* 'env' parameter? */
                lua_pushvalue(L, Env);  /* environment for loaded function */
                if (!lua_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
                {
                    lua_pop(L, 1);  /* remove 'env' if not used by previous call */
                }
            }
        }
        else
        {
            lua_pushnil(L);     /* error (message is on top of the stack) */
            lua_insert(L, -2);  /* put before error message */
        }

        return Code == LUA_OK;
    }

    /**
     * Run a Lua chunk
     */
    bool RunChunk(lua_State *L, const char *Chunk)
    {
        if (!Chunk)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid lua chunk!"), ANSI_TO_TCHAR(__FUNCTION__));
            return false;
        }

        const auto& Env = FLuaEnv::FindEnvChecked(L);
        const auto DanglingGuard = Env.GetDanglingCheck()->MakeGuard();
        bool bSuccess = !luaL_dostring(L, Chunk);       // loads and runs the given chunk
        if (!bSuccess)
        {
            ReportLuaCallError(L);
            return false;
        }

        return bSuccess;
    }

    /**
     * Report Lua error
     */
    int32 ReportLuaCallError(lua_State *L)
    {
        if (FUnLuaDelegates::ReportLuaCallError.IsBound())
        {
            return FUnLuaDelegates::ReportLuaCallError.Execute(L);      // developers can provide error reporter themselves
        }

        int32 Type = lua_type(L, -1);
        if (Type == LUA_TSTRING)
        {
            const char *ErrorString = lua_tostring(L, -1);
            luaL_traceback(L, L, ErrorString, 1);
            ErrorString = lua_tostring(L, -1);
            UE_LOG(LogUnLua, Error, TEXT("Lua error message: %s"), UTF8_TO_TCHAR(ErrorString));
        }
        else if (Type == LUA_TTABLE)
        {
            // multiple errors is possible
            int32 MessageIndex = 0;
            lua_pushnil(L);
            while (lua_next(L, -2) != 0)
            {
                const char *ErrorString = lua_tostring(L, -1);
                UE_LOG(LogUnLua, Error, TEXT("Lua error message %d : %s"), MessageIndex++, UTF8_TO_TCHAR(ErrorString));
                lua_pop(L, 1);
            }
        }

        lua_pop(L, 1);
        return 0;
    }

    /**
     * Push a pointer with the name of meta table
     */
    int32 PushPointer(lua_State *L, void *Value, const char *MetatableName, bool bAlwaysCreate)
    {
        if (!Value
            || !MetatableName)
        {
            lua_pushnil(L);
            return 1;
        }

        bool bCreateUserdata = bAlwaysCreate;
        if (!bAlwaysCreate)
        {
            // find the pointer from 'StructMap' first
            lua_getfield(L, LUA_REGISTRYINDEX, "StructMap");
            lua_pushlightuserdata(L, Value);
            int32 Type = lua_rawget(L, -2);
            if (Type == LUA_TUSERDATA)
            {
                lua_remove(L, -2);

                // check metatable is same?
                bool bMTSame = false;
                if (lua_getmetatable(L, -1))
                {   
                    luaL_getmetatable(L, MetatableName);
                    if (lua_rawequal(L,-1,-2))
                    {   
                        bMTSame = true;
                    }

                    lua_pop(L, 2);
                }

				if (!bMTSame)
                {
#if UNLUA_ENABLE_DEBUG != 0
                    FString CurMetatableName;
                    if (lua_getmetatable(L, -1))
                    {
                        lua_pushstring(L, "__name");
                        Type = lua_rawget(L,-2);
                        if (LUA_TSTRING == Type)
                        {
                            CurMetatableName = UTF8_TO_TCHAR(lua_tostring(L,-1));
                        }
                        lua_pop(L, 2);
                    }
					UE_LOG(LogTemp, Log, TEXT("%s : userdata with difference metatable finded! need %s,get %s,may be local or stack variable pushed to lua..."),
                        ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(MetatableName), *CurMetatableName);
#endif
                    bool bSuccess = TryToSetMetatable(L, MetatableName);        // set metatable
                    if (!bSuccess)
                    {
                        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable, metatable name: %s!"),  UTF8_TO_TCHAR(MetatableName));
                        return 1;
                    }
                }
            }
            else
            {
                check(Type == LUA_TNIL);
                lua_pop(L, 1);
                bCreateUserdata = true;     // create a new userdata if the value is not found
            }
        }

        if (bCreateUserdata)
        {
            NewUserdataWithTwoLvPtrTag(L, sizeof(void*), Value);
            if (MetatableName)
            {
                bool bSuccess = TryToSetMetatable(L, MetatableName);        // set metatable
                if (!bSuccess)
                {
                    UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable, metatable name: %s!"), ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(MetatableName));
                    lua_pop(L, 1);
                    return 1;
                }
            }

            if (!bAlwaysCreate)
            {
                // cache the new userdata in 'StructMap
                FLuaEnv::FindEnv(L)->GetDanglingCheck()->CaptureStruct(L, Value);
                lua_pushlightuserdata(L, Value);
                lua_pushvalue(L, -2);
                lua_rawset(L, -4);
                lua_remove(L, -2);
            }
        }

        return 1;
    }

    /**
     * Get the address of user data at the given stack index
     */
    void* GetPointer(lua_State *L, int32 Index, bool *OutTwoLvlPtr)
    {
        bool bTwoLvlPtr = false;
        void *Userdata = ::GetUserdataFast(L, Index, &bTwoLvlPtr);
        if (Userdata)
        {
            if (OutTwoLvlPtr)
            {
                *OutTwoLvlPtr = bTwoLvlPtr;
            }
            return bTwoLvlPtr ? *((void**)Userdata) : Userdata;     // return the address
        }
        return nullptr;
    }

    /**
     * Push a UObject
     */
    int32 PushUObject(lua_State *L, UObjectBaseUtility *Object, bool bAddRef)
    {
        FLuaEnv::FindEnvChecked(L).GetObjectRegistry()->Push(L, (UObject*)Object);
        return 1;
    }

    /**
     * Get a UObject at the given stack index
     */
    UObject* GetUObject(lua_State *L, int32 Index, bool bReturnNullIfInvalid)
    {
        UObject* Object = (UObject*)GetCppInstance(L, Index);
        if (UNLIKELY(bReturnNullIfInvalid && !IsUObjectValid(Object)))
            return nullptr;
        return Object;
    }

    /**
     * Allocate user data for smart pointer
     */
    void* NewSmartPointer(lua_State *L, int32 Size, const char *MetatableName)
    {
        void *Userdata = ::NewUserdataWithPadding(L, Size, MetatableName);
        if (Userdata)
        {
            MarkUserdataTwoLvPtrTag(Userdata);       // mark the new userdata as a two level pointer
        }
        return Userdata;
    }

    /**
     * Get the address of smart pointer at the given stack index
     */
    void* GetSmartPointer(lua_State *L, int32 Index)
    {
        bool bTwoLvlPtr = false;
        void *Userdata = ::GetUserdataFast(L, Index, &bTwoLvlPtr);
        return Userdata;
    }

    /**
     * Allocate user data
     */
    void* NewUserdata(lua_State *L, int32 Size, const char *MetatableName, int32 Alignment)
    {
        return ::NewUserdataWithPadding(L, Size, MetatableName, CalcUserdataPadding(Alignment));
    }

    /**
     * Push an untyped dynamic array (same memory layout with TArray)
     */
    int32 PushArray(lua_State *L, const FScriptArray *ScriptArray, TSharedPtr<ITypeInterface> TypeInterface, bool bCreateCopy)
    {
        if (!L || !ScriptArray || !TypeInterface)
        {
            return 0;
        }

        const auto Registry = FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        if (bCreateCopy)
        {
            int32 Num = ScriptArray->Num();
            int32 ElementSize = TypeInterface->GetSize();

            const auto LuaArray = Registry->NewArray(L, TypeInterface, FLuaArray::OwnedBySelf);
            FScriptArray *DestScriptArray = LuaArray->GetContainerPtr();       // create a new FScriptArray

#if ENGINE_MAJOR_VERSION >=5
            DestScriptArray->Empty(Num, ElementSize, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
            DestScriptArray->Add(Num, ElementSize, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
#else
            DestScriptArray->Empty(Num, ElementSize);
            DestScriptArray->Add(Num, ElementSize);
#endif

            if (Num)
            {
                uint8 *SrcData = (uint8*)ScriptArray->GetData();
                uint8 *DestData = (uint8*)DestScriptArray->GetData();
                for (int32 i = 0; i < Num; ++i)
                {
                    TypeInterface->Initialize(DestData);
                    TypeInterface->Copy(DestData, SrcData);     // copy data
                    SrcData += ElementSize;
                    DestData += ElementSize;
                }
            }
        }
        else
        {
            // get the cached array or create a new one if not found
            Registry->FindOrAdd(L, (FScriptArray*)ScriptArray, TypeInterface);
        }
        return 1;
    }

    /**
     * Push an untyped set (same memory layout with TSet). see PushArray
     */
    int32 PushSet(lua_State *L, const FScriptSet *ScriptSet, TSharedPtr<ITypeInterface> TypeInterface, bool bCreateCopy)
    {
        if (!L || !ScriptSet || !TypeInterface)
        {
            return 0;
        }

        const auto Registry = FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        if (bCreateCopy)
        {
            int32 Num = ScriptSet->Num();
            FLuaSet SrcSet(ScriptSet, TypeInterface, FLuaSet::OwnedByOther);
            FLuaSet* DestSet = Registry->NewSet(L, TypeInterface, FLuaSet::OwnedBySelf);
            DestSet->Clear(Num);
            for (int32 SrcIndex = 0; Num; ++SrcIndex)
            {
                if (ScriptSet->IsValidIndex(SrcIndex))
                {
                    const int32 DestIndex = DestSet->AddDefaultValue_Invalid_NeedsRehash();
                    uint8 *SrcData = SrcSet.GetData(SrcIndex);
                    uint8 *DestData = DestSet->GetData(DestIndex);
                    TypeInterface->Copy(DestData, SrcData);
                    --Num;
                }
            }
            DestSet->Rehash();
        }
        else
        {
            Registry->FindOrAdd(L, (FScriptSet*)ScriptSet, TypeInterface);
        }
        return 1;
    }

    /**
     * Push an untyped map (same memory layout with TMap). see PushArray
     */
    int32 PushMap(lua_State *L, const FScriptMap *ScriptMap, TSharedPtr<ITypeInterface> KeyInterface, TSharedPtr<ITypeInterface> ValueInterface, bool bCreateCopy)
    {
        if (!L || !ScriptMap || !KeyInterface || !ValueInterface)
        {
            return 0;
        }

        const auto Registry = FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        if (bCreateCopy)
        {
            int32 Num = ScriptMap->Num();
            FLuaMap SrcMap(ScriptMap, KeyInterface, ValueInterface, FLuaMap::OwnedByOther);
            FLuaMap *DestMap = Registry->NewMap(L, KeyInterface, ValueInterface, FLuaMap::OwnedBySelf);
            DestMap->Clear(Num);
            for (int32 SrcIndex = 0; Num; ++SrcIndex)
            {
                if (ScriptMap->IsValidIndex(SrcIndex))
                {
                    int32 DestIndex = DestMap->AddDefaultValue_Invalid_NeedsRehash();
                    uint8 *SrcData = SrcMap.GetData(SrcIndex);
                    uint8 *DestData = DestMap->GetData(DestIndex);
                    KeyInterface->Copy(DestData, SrcData);
                    ValueInterface->Copy(DestData + DestMap->MapLayout.ValueOffset, SrcData + SrcMap.MapLayout.ValueOffset);
                    --Num;
                }
            }
            DestMap->Rehash();
        }
        else
        {
            Registry->FindOrAdd(L, (FScriptMap*)ScriptMap, KeyInterface, ValueInterface);
        }
        return 1;
    }

    /**
     * Get an untyped dynamic array at the given stack index
     */
    FScriptArray* GetArray(lua_State *L, int32 Index)
    {
        FScriptArray *ScriptArray = (FScriptArray*)GetScriptContainer(L, Index);
        return ScriptArray;
    }

    /**
     * Get an untyped set at the given stack index
     */
    FScriptSet* GetSet(lua_State *L, int32 Index)
    {
        FScriptSet *ScriptSet = (FScriptSet*)GetScriptContainer(L, Index);
        return ScriptSet;
    }

    /**
     * Get an untyped map at the given stack index
     */
    FScriptMap* GetMap(lua_State *L, int32 Index)
    {
        FScriptMap *ScriptMap = (FScriptMap*)GetScriptContainer(L, Index);
        return ScriptMap;
    }

    /**
     * Helper to recover Lua stack automatically
     */
    FAutoStack::FAutoStack(lua_State* L): L(L)
    {
        OldTop = -1;
        if (L)
        {
            OldTop = lua_gettop(L);
        }
    }

    FAutoStack::~FAutoStack()
    {
        if ((L)
            && (-1 != OldTop))
        {
            lua_settop(L, OldTop);
        }
    }

} // namespace UnLua
