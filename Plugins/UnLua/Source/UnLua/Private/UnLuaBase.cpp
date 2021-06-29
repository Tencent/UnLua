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

#include "LuaCore.h"
#include "LuaContext.h"
#include "UnLuaDelegates.h"
#include "UEObjectReferencer.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY(LogUnLua);

namespace UnLua
{

    bool AddTypeInterface(FName Name, TSharedPtr<ITypeInterface> TypeInterface)
    {
        FLuaContext::Create();
        return GLuaCxt->AddTypeInterface(Name, TypeInterface);
    }

    IExportedClass* FindExportedClass(FName Name)
    {
        FLuaContext::Create();
        return GLuaCxt->FindExportedClass(Name);
    }

    bool ExportClass(IExportedClass *Class)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportClass(Class);
    }

    bool ExportFunction(IExportedFunction *Function)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportFunction(Function);
    }

    bool ExportEnum(IExportedEnum *Enum)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportEnum(Enum);
    }

    lua_State* CreateState()
    {
        if (GLuaCxt)
        {
            GLuaCxt->CreateState();
            return *GLuaCxt;
        }
        return nullptr;
    }

    lua_State* GetState()
    {
        return GLuaCxt ? (lua_State*)(*GLuaCxt) : nullptr;
    }

    bool Startup()
    {
        if (GLuaCxt)
        {
            GLuaCxt->SetEnable(true);
            return true;
        }
        return false;
    }

    void Shutdown()
    {
        if (GLuaCxt)
        {
            GLuaCxt->SetEnable(false);
        }
    }

    /**
     * Load a Lua file without running it
     */
    bool LoadFile(lua_State *L, const FString &RelativeFilePath, const char *Mode, int32 Env)
    {
        FString FullFilePath = GLuaSrcFullPath + RelativeFilePath;
        FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FString ProjectPersistentDownloadDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPersistentDownloadDir());
        if (!ProjectPersistentDownloadDir.EndsWith("/"))
        {
            ProjectPersistentDownloadDir.Append("/");
        }
        FString RealFilePath = FullFilePath.Replace(*ProjectDir, *ProjectPersistentDownloadDir);        // try to load the file from 'ProjectPersistentDownloadDir' first
        if (!IFileManager::Get().FileExists(*RealFilePath))
        {
            RealFilePath = FullFilePath;
        }
        else
        {
            UE_LOG(LogUnLua, Log, TEXT("Load lua file from DownloadDir : %s"), *RealFilePath);
        }

        TArray<uint8> Data;
        // developers can provide a delegate to load the file
        bool bSuccess = FUnLuaDelegates::LoadLuaFile.IsBound() ? FUnLuaDelegates::LoadLuaFile.Execute(RealFilePath, Data) : FFileHelper::LoadFileToArray(Data, *RealFilePath, 0);
        if (!bSuccess)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to load lua file!"), ANSI_TO_TCHAR(__FUNCTION__));
            return false;
        }

        int32 SkipLen = (3 < Data.Num()) && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0;        // skip UTF-8 BOM mark
        return LoadChunk(L, (const char*)(Data.GetData() + SkipLen), Data.Num() - SkipLen, TCHAR_TO_ANSI(*RelativeFilePath), Mode, Env);    // loads the buffer as a Lua chunk
    }

    /**
     * Run a Lua file
     */
    bool RunFile(lua_State *L, const FString &RelativeFilePath, const char *Mode, int32 Env)
    {
        bool bSuccess = LoadFile(L, RelativeFilePath, Mode, Env);       // load the file content as a Lua chunk
        if (!bSuccess)
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to load lua file!"), ANSI_TO_TCHAR(__FUNCTION__));
            return false;
        }

        int32 Code = lua_pcall(L, 0, LUA_MULTRET, 0);       // pcall
        if (Code != LUA_OK)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Failed to call lua_pcall, error code: %d"), Code);
            ReportLuaCallError(L);                          // report pcall error
        }

        return Code == LUA_OK;
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
            UE_LOG(LogUnLua, Warning, TEXT("Lua error message: %s"), UTF8_TO_TCHAR(ErrorString));
        }
        else if (Type == LUA_TTABLE)
        {
            // multiple errors is possible
            int32 MessageIndex = 0;
            lua_pushnil(L);
            while (lua_next(L, -2) != 0)
            {
                const char *ErrorString = lua_tostring(L, -1);
                UE_LOG(LogUnLua, Warning, TEXT("Lua error message %d : %s"), MessageIndex++, UTF8_TO_TCHAR(ErrorString));
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
        if (!Value)
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
            void **Userdata = (void**)lua_newuserdata(L, sizeof(void*));
            *Userdata = Value;
            if (MetatableName)
            {
                bool bSuccess = TryToSetMetatable(L, MetatableName);        // set metatable
                if (!bSuccess)
                {
                    UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable, metatable name: !"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(MetatableName));
                    return 1;
                }
            }
            MarkUserdataTwoLvlPtr(L, -1);       // mark the userdata as a two level pointer

            if (!bAlwaysCreate)
            {
                // cache the new userdata in 'StructMap
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
        if (!Object)
        {
            lua_pushnil(L);
            return 1;
        }

        lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");
        lua_pushlightuserdata(L, Object);
        int32 Type = lua_rawget(L, -2);     // find the object from 'ObjectMap' first
        if (Type == LUA_TNIL)
        {
            // 1. create a new userdata for the object if it's not found; 2. cache it in 'ObjectMap'
            lua_pop(L, 1);
            PushObjectCore(L, Object);
            lua_pushlightuserdata(L, Object);
            lua_pushvalue(L, -2);
            lua_rawset(L, -4);

            if (bAddRef && !Object->IsNative())
            {
                GObjectReferencer.AddObjectRef((UObject*)Object);       // add a reference for the object if it's a non-native object
            }
        }
        lua_remove(L, -2);

        return 1;
    }

    /**
     * Get a UObject at the given stack index
     */
    UObject* GetUObject(lua_State *L, int32 Index)
    {
#if UE_BUILD_DEBUG
        if (lua_getmetatable(L, Index) == 0)
        {
            return nullptr;
        }
        // todo: stricter check?
        lua_pushstring(L, "__name");
        lua_gettable(L, -2);
        const char *ClassName = lua_tostring(L, -1);
        FClassDesc *ClassDesc = GReflectionRegistry.FindClass(ClassName);
        lua_pop(L, 2);
        return ClassDesc && ClassDesc->IsClass() ? (UObject*)GetCppInstance(L, Index) : nullptr;
#else
        return (UObject*)GetCppInstance(L, Index);
#endif
    }

    /**
     * Allocate user data for smart pointer
     */
    void* NewSmartPointer(lua_State *L, int32 Size, const char *MetatableName)
    {
        void *Userdata = ::NewUserdataWithPadding(L, Size, MetatableName);
        if (Userdata)
        {
            MarkUserdataTwoLvlPtr(L, -1);       // mark the new userdata as a two level pointer
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

        if (bCreateCopy)
        {
            int32 Num = ScriptArray->Num();
            int32 ElementSize = TypeInterface->GetSize();
            FScriptArray *DestScriptArray = new FScriptArray;       // create a new FScriptArray
            DestScriptArray->Empty(Num, ElementSize);
            DestScriptArray->Add(Num, ElementSize);
            if (Num)
            {
                uint8 *SrcData = (uint8*)ScriptArray->GetData();
                uint8 *DestData = (uint8*)DestScriptArray->GetData();
                for (int32 i = 0; i < Num; ++i)
                {
                    TypeInterface->Copy(DestData, SrcData);     // copy data
                    SrcData += ElementSize;
                    DestData += ElementSize;
                }
            }

            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Array);                // create a new userdata for the new created FScriptArray
            new(Userdata) FLuaArray(DestScriptArray, TypeInterface, FLuaArray::OwnedBySelf);    // placement new
        }
        else
        {
            // get the cached array or create a new one if not found
            void *Userdata = CacheScriptContainer(L, (FScriptArray*)ScriptArray, FScriptContainerDesc::Array);
            if (Userdata)
            {
                FLuaArray *LuaArray = new(Userdata) FLuaArray(ScriptArray, TypeInterface, FLuaArray::OwnedByOther);     // placement new
            }
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

        if (bCreateCopy)
        {
            int32 Num = ScriptSet->Num();
            FLuaSet SrcSet(ScriptSet, TypeInterface, FLuaSet::OwnedByOther);
            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Set);
            FLuaSet *DestSet = new(Userdata) FLuaSet(new FScriptSet, TypeInterface, FLuaSet::OwnedBySelf);
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
            void *Userdata = CacheScriptContainer(L, (FScriptSet*)ScriptSet, FScriptContainerDesc::Set);
            if (Userdata)
            {
                FLuaSet *LuaSet = new(Userdata) FLuaSet(ScriptSet, TypeInterface, FLuaSet::OwnedByOther);
            }
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

        if (bCreateCopy)
        {
            int32 Num = ScriptMap->Num();
            FLuaMap SrcMap(ScriptMap, KeyInterface, ValueInterface, FLuaMap::OwnedByOther);
            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Map);
            FLuaMap *DestMap = new(Userdata) FLuaMap(new FScriptMap, KeyInterface, ValueInterface, FLuaMap::OwnedBySelf);
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
            void *Userdata = CacheScriptContainer(L, (FScriptMap*)ScriptMap, FScriptContainerDesc::Map);
            if (Userdata)
            {
                FLuaMap *LuaMap = new(Userdata) FLuaMap(ScriptMap, KeyInterface, ValueInterface, FLuaMap::OwnedByOther);
            }
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

} // namespace UnLua
