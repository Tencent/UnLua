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

#include "CoreUObject.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UnLuaCompatibility.h"

#ifndef AUTO_UNLUA_STARTUP
#define AUTO_UNLUA_STARTUP 0
#endif

#ifndef WITH_UE4_NAMESPACE
#define WITH_UE4_NAMESPACE 0
#endif

#ifndef SUPPORTS_RPC_CALL
#define SUPPORTS_RPC_CALL 0
#endif



UNLUA_API DECLARE_LOG_CATEGORY_EXTERN(LogUnLua, Log, All);

#if UNLUA_UE_VER < 425
typedef UProperty FProperty;
#endif

struct lua_State;
struct luaL_Reg;

namespace UnLua
{   
    //!!!Fix!!!

    /**
     * Interface to manage Lua stack for a C++ type
     */
    struct ITypeOps
    {   
        ITypeOps() { StaticExported = false; };
        
        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const = 0;
        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const = 0;

        bool StaticExported;
    };

    /**
     * Interface to manage a C++ type
     */
    struct ITypeInterface : public ITypeOps
    {
        ITypeInterface() { }
        virtual ~ITypeInterface() {}

        virtual bool IsPODType() const = 0;
        virtual bool IsTriviallyDestructible() const = 0;
        virtual int32 GetSize() const = 0;
        virtual int32 GetAlignment() const = 0;
        virtual int32 GetOffset() const = 0;
        virtual uint32 GetValueTypeHash(const void *Src) const = 0;
        virtual void Initialize(void *Dest) const = 0;
        virtual void Destruct(void *Dest) const = 0;
        virtual void Copy(void *Dest, const void *Src) const = 0;
        virtual bool Identical(const void *A, const void *B) const = 0;
        virtual FString GetName() const = 0;
        virtual FProperty* GetUProperty() const = 0;
    };

    /**
     * Exported property interface
     */
    struct IExportedProperty : public ITypeOps
    {   
        IExportedProperty() { StaticExported = true;}
		virtual ~IExportedProperty() {}
       

        virtual void Register(lua_State *L) = 0;

#if WITH_EDITOR
        virtual FString GetName() const = 0;
        virtual void GenerateIntelliSense(FString &Buffer) const = 0;
#endif
    };

    /**
     * Exported function interface
     */
    struct IExportedFunction
    {
        virtual ~IExportedFunction() {}

        virtual void Register(lua_State *L) = 0;
        virtual int32 Invoke(lua_State *L) = 0;

#if WITH_EDITOR
        virtual FString GetName() const = 0;
        virtual void GenerateIntelliSense(FString &Buffer) const = 0;
#endif
    };

    /**
     * Exported class interface
     */
    struct IExportedClass
    {
        virtual ~IExportedClass() {}

        virtual void Register(lua_State *L) = 0;
        virtual void AddLib(const luaL_Reg *Lib) = 0;
        virtual bool IsReflected() const = 0;
        virtual FName GetName() const = 0;

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const = 0;
#endif
    };

    /**
     * Exported enum interface
     */
    struct IExportedEnum
    {
        virtual ~IExportedEnum() {}

        virtual void Register(lua_State *L) = 0;

#if WITH_EDITOR
        virtual FString GetName() const = 0;
        virtual void GenerateIntelliSense(FString &Buffer) const = 0;
#endif
    };


    /**
     * Add type info
     *
     * @param Name - name of the type
     * @param TypeInterface - instance of the type info
     * @return - true if type interface is added successfully, false otherwise
     */
    UNLUA_API bool AddTypeInterface(FName Name, TSharedPtr<ITypeInterface> TypeInterface);

    /**
     * Find the exported class with its name
     *
     * @param Name - name of the exported class
     * @return - the exported class
     */
    UNLUA_API IExportedClass* FindExportedClass(FName Name);

    /**
     * Export a class
     *
     * @param Class - exported class instance
     * @return - true if the class is exported successfully, false otherwise
     */
    UNLUA_API bool ExportClass(IExportedClass *Class);

    /**
     * Export a global function
     *
     * @param Function - exported function instance
     * @return - true if the global function is exported successfully, false otherwise
     */
    UNLUA_API bool ExportFunction(IExportedFunction *Function);

    /**
     * Export an enum
     *
     * @param Enum - exported enum instance
     * @return - true if the enum is exported successfully, false otherwise
     */
    UNLUA_API bool ExportEnum(IExportedEnum *Enum);


    /**
     * Create Lua state
     *
     * @return - created Lua state
     */
    UNLUA_API lua_State* CreateState();

    /**
     * @return - Lua state
     */
    UNLUA_API lua_State* GetState();

    /**
     * Start up UnLua
     *
     * @return - true if UnLua is started successfully, false otherwise
     */
    UNLUA_API bool Startup();

    /**
     * Shut down UnLua
     */
    UNLUA_API void Shutdown();

    /**
     * Load a Lua file without running it
     *
     * @param RelativeFilePath - the relative (to project's content dir) Lua file path
     * @param Mode - mode of the chunk, it may be the string "b" (only binary chunks), "t" (only text chunks), or "bt" (both binary and text)
     * @param Env - Lua stack index of the 'Env'
     * @return - true if Lua file is loaded successfully, false otherwise
     */
    UNLUA_API bool LoadFile(lua_State *L, const FString &RelativeFilePath, const char *Mode = "bt", int32 Env = 0);

    /**
     * Run a Lua file
     *
     * @param RelativeFilePath - the relative (to project's content dir) Lua file path
     * @param Mode - mode of the chunk, it may be the string "b" (only binary chunks), "t" (only text chunks), or "bt" (both binary and text)
     * @param Env - Lua stack index of the 'Env'
     * @return - true if the Lua file runs successfully, false otherwise
     */
    UNLUA_API bool RunFile(lua_State *L, const FString &RelativeFilePath, const char *Mode = "bt", int32 Env = 0);

    /**
     * Load a Lua chunk without running it
     *
     * @param Chunk - Lua chunk
     * @param ChunkSize - chunk size (in bytes)
     * @param ChunkName - name of the chunk, which is used for error messages and in debug information
     * @param Mode - mode of the chunk, it may be the string "b" (only binary chunks), "t" (only text chunks), or "bt" (both binary and text)
     * @param Env - Lua stack index of the 'Env'
     * @return - true if Lua chunk is loaded successfully, false otherwise
     */
    UNLUA_API bool LoadChunk(lua_State *L, const char *Chunk, int32 ChunkSize, const char *ChunkName = "", const char *Mode = "bt", int32 Env = 0);

    /**
     * Run a Lua chunk
     *
     * @param Chunk - Lua chunk
     * @return - true if the Lua chunk runs successfully, false otherwise
     */
    UNLUA_API bool RunChunk(lua_State *L, const char *Chunk);

    /**
     * Report Lua error
     *
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 ReportLuaCallError(lua_State *L);

    /**
     * Push a pointer with the name of meta table
     *
     * @param Value - pointer address
     * @param MetatableName - name of the meta table
     * @param bAlwaysCreate - always create Lua instance for this pointer
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 PushPointer(lua_State *L, void *Value, const char *MetatableName, bool bAlwaysCreate = false);

    /**
     * Get the address of user data at the given stack index
     *
     * @param Index - Lua stack index
     * @param[out] OutTwoLvlPtr - whether the address is a two level pointer
     * @return - the address of user data
     */
    UNLUA_API void* GetPointer(lua_State *L, int32 Index, bool *OutTwoLvlPtr = nullptr);

    /**
     * Push a UObject
     *
     * @param Object - UObject instance
     * @param bAddRef - whether to add reference for this object
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 PushUObject(lua_State *L, UObjectBaseUtility *Object, bool bAddRef = true);

    /**
     * Get a UObject at the given stack index
     *
     * @param Index - Lua stack index
     * @return - the number of results on Lua stack
     */
    UNLUA_API UObject* GetUObject(lua_State *L, int32 Index);

    /**
     * Allocate user data for smart pointer
     *
     * @param Size - user data size (in bytes)
     * @param MetatableName - name of the meta table
     * @return - the address of new created user data
     */
    UNLUA_API void* NewSmartPointer(lua_State *L, int32 Size, const char *MetatableName);

    /**
     * Get the address of smart pointer at the given stack index
     *
     * @param Index - Lua stack index
     * @return - the address of the smart pointer
     */
    UNLUA_API void* GetSmartPointer(lua_State *L, int32 Index);

    /**
     * Allocate user data
     *
     * @param Size - memory size (in bytes)
     * @param MetatableName - name of the meta table
     * @param Alignment - alignment (in bytes)
     * @return - the address of new created user data
     */
    UNLUA_API void* NewUserdata(lua_State *L, int32 Size, const char *MetatableName, int32 Alignment);

    /**
     * Push an untyped dynamic array (same memory layout with TArray)
     *
     * @param ScriptArray - untyped dynamic array
     * @param TypeInterface - type info for the dynamic array
     * @param bCreateCopy - whether to copy the dynamic array
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 PushArray(lua_State *L, const FScriptArray *ScriptArray, TSharedPtr<ITypeInterface> TypeInterface, bool bCreateCopy = false);

    /**
     * Push an untyped set (same memory layout with TSet)
     *
     * @param ScriptSet - untyped set
     * @param TypeInterface - type info for the set
     * @param bCreateCopy - whether to copy the set
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 PushSet(lua_State *L, const FScriptSet *ScriptSet, TSharedPtr<ITypeInterface> TypeInterface, bool bCreateCopy = false);

    /**
     * Push an untyped map (same memory layout with TMap)
     *
     * @param ScriptMap - untyped map
     * @param TypeInterface - type info for the map
     * @param bCreateCopy - whether to copy the map
     * @return - the number of results on Lua stack
     */
    UNLUA_API int32 PushMap(lua_State *L, const FScriptMap *ScriptMap, TSharedPtr<ITypeInterface> KeyInterface, TSharedPtr<ITypeInterface> ValueInterface, bool bCreateCopy = false);

    /**
     * Get an untyped dynamic array at the given stack index
     *
     * @param Index - Lua stack index
     * @return - the untyped dynamic array
     */
    UNLUA_API FScriptArray* GetArray(lua_State *L, int32 Index);

    /**
     * Get an untyped set at the given stack index
     *
     * @param Index - Lua stack index
     * @return - the untyped set
     */
    UNLUA_API FScriptSet* GetSet(lua_State *L, int32 Index);

    /**
     * Get an untyped map at the given stack index
     *
     * @param Index - Lua stack index
     * @return - the untyped map
     */
    UNLUA_API FScriptMap* GetMap(lua_State *L, int32 Index);


    /**
     * Helper to recover Lua stack automatically
     */
    struct UNLUA_API FAutoStack
    {
        FAutoStack();
        ~FAutoStack();

    private:
        int32 OldTop;
    };
} // namespace UnLua
