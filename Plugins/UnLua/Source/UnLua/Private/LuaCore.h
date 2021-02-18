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

#include "UnLuaPrivate.h"
#include "UnLuaCompatibility.h"
#include "lua.hpp"

struct FScriptContainerDesc
{
    FORCEINLINE int32 GetSize() const { return Size; }
    FORCEINLINE const char* GetName() const { return Name; }

    static const FScriptContainerDesc Array;
    static const FScriptContainerDesc Set;
    static const FScriptContainerDesc Map;

private:
    FORCEINLINE FScriptContainerDesc(int32 InSize, const char *InName)
        : Size(InSize), Name(InName)
    {}

    int32 Size;
    const char *Name;
};

void CreateNamespaceForUE(lua_State *L);
void SetTableForClass(lua_State *L, const char *Name);

/**
 * Set metatable for the userdata/table on the top of the stack
 */
bool TryToSetMetatable(lua_State *L, const char *MetatableName);

/**
 * Functions to handle Lua userdata
 */
uint8 CalcUserdataPadding(int32 Alignment);
template <typename T> uint8 CalcUserdataPadding() { return CalcUserdataPadding(alignof(T)); }
void SetUserdataPadding(lua_State *L, int32 Index, uint8 PaddingBytes);
void MarkUserdataTwoLvlPtr(lua_State *L, int32 Index);
void* GetUserdata(lua_State *L, int32 Index, bool *OutTwoLvlPtr = nullptr, bool *OutClassMetatable = nullptr);
void* GetUserdataFast(lua_State *L, int32 Index, bool *OutTwoLvlPtr = nullptr);
void* NewUserdataWithPadding(lua_State *L, int32 Size, const char *MetatableName, uint8 PaddingSize = 0);
#define NewTypedUserdata(L, Type) NewUserdataWithPadding(L, sizeof(Type), #Type, CalcUserdataPadding<Type>())
void* GetCppInstance(lua_State *L, int32 Index);
void* GetCppInstanceFast(lua_State *L, int32 Index);

/**
 * Functions to handle script containers
 */
void* NewScriptContainer(lua_State *L, const FScriptContainerDesc &Desc);
void* CacheScriptContainer(lua_State *L, void *Key, const FScriptContainerDesc &Desc);
void* GetScriptContainer(lua_State *L, int32 Index);
void* GetScriptContainer(lua_State *L, void *Key);
void RemoveCachedScriptContainer(lua_State *L, void *Key);

/**
 * Functions to push FProperty array
 */
void PushIntegerArray(lua_State *L, FNumericProperty *Property, void *Value);
void PushFloatArray(lua_State *L, FNumericProperty *Property, void *Value);
void PushEnumArray(lua_State *L, FNumericProperty *Property, void *Value);
void PushFNameArray(lua_State *L, FNameProperty *Property, void *Value);
void PushFStringArray(lua_State *L, FStrProperty *Property, void *Value);
void PushFTextArray(lua_State *L, FTextProperty *Property, void *Value);
void PushObjectArray(lua_State *L, FObjectPropertyBase *Property, void *Value);
void PushInterfaceArray(lua_State *L, FInterfaceProperty *Property, void *Value);
void PushDelegateArray(lua_State *L, FDelegateProperty *Property, void *Value);
void PushMCDelegateArray(lua_State *L, FMulticastDelegateProperty *Property, void *Value, const char *MetatableName);
void PushStructArray(lua_State *L, FProperty *Property, void *Value, const char *MetatableName);

/**
 * Push a UObject to Lua stack
 */
void PushObjectCore(lua_State *L, UObjectBaseUtility *Object);

/**
 * Functions to New/Delete Lua instance for UObjectBaseUtility
 */
int32 NewLuaObject(lua_State *L, UObjectBaseUtility *Object, UClass *Class, const char *ModuleName);
void DeleteLuaObject(lua_State *L, UObjectBaseUtility *Object);

/**
 * Get UObject and Lua function address for delegate
 */
int32 GetDelegateInfo(lua_State *L, int32 Index, UObject* &Object, const void* &Function);

/**
 * Functions to handle Lua functions
 */
bool GetFunctionList(lua_State *L, const char *InModuleName, TSet<FName> &FunctionNames);
int32 PushFunction(lua_State *L, UObjectBaseUtility *Object, const char *FunctionName);
bool PushFunction(lua_State *L, UObjectBaseUtility *Object, int32 FunctionRef);
bool CallFunction(lua_State *L, int32 NumArgs, int32 NumResults);

/**
 * Get corresponding Lua instance for a UObjectBaseUtility
 */
UNLUA_API bool GetObjectMapping(lua_State *L, UObjectBaseUtility *Object);

/**
 * Add Lua package path
 */
UNLUA_API void AddPackagePath(lua_State *L, const char *Path);

/**
 * Functions to handle loaded Lua module
 */
void ClearLoadedModule(lua_State *L, const char *ModuleName);
int32 GetLoadedModule(lua_State *L, const char *ModuleName);

/**
 * Functions to register collision enums
 */
bool RegisterECollisionChannel(lua_State *L);
bool RegisterEObjectTypeQuery(lua_State *L);
bool RegisterETraceTypeQuery(lua_State *L);

void ClearLibrary(lua_State *L, const char *LibrayName);

/**
 * Functions to create weak table
 */
void CreateWeakKeyTable(lua_State *L);
void CreateWeakValueTable(lua_State *L);

int32 TraverseTable(lua_State *L, int32 Index, void *Userdata, bool (*TraverseWorker)(lua_State*, void*));
bool PeekTableElement(lua_State *L, void *Userdata);

/**
 * Functions to register UEnum
 */
int32 Global_RegisterEnum(lua_State *L);
bool RegisterEnum(lua_State *L, const char *EnumName);
bool RegisterEnum(lua_State *L, UEnum *Enum);

/**
 * Functions to register UClass
 */
int32 Global_RegisterClass(lua_State *L);
class FClassDesc* RegisterClass(lua_State *L, const char *ClassName, const char *SuperClassName = nullptr);
class FClassDesc* RegisterClass(lua_State *L, UStruct *Struct, UStruct *SuperStruct = nullptr);

/**
 * Lua global functions
 */
int32 Global_GetUProperty(lua_State *L);
int32 Global_SetUProperty(lua_State *L);
int32 Global_LoadObject(lua_State *L);
int32 Global_LoadClass(lua_State *L);
int32 Global_NewObject(lua_State *L);
UNLUA_API int32 Global_Print(lua_State *L);
UNLUA_API int32 Global_Require(lua_State *L);

/**
 * Functions to handle UEnum
 */
int32 Enum_Index(lua_State *L);
int32 Enum_Delete(lua_State *L);

/**
 * Functions to handle UClass
 */
int32 Class_Index(lua_State* L);
int32 Class_NewIndex(lua_State* L);
int32 Class_CallUFunction(lua_State *L);
int32 Class_CallLatentFunction(lua_State *L);
int32 Class_StaticClass(lua_State *L);
int32 Class_Cast(lua_State* L);

/**
 * Functions to handle UScriptStruct
 */
int32 ScriptStruct_New(lua_State *L);
int32 ScriptStruct_Delete(lua_State *L);
int32 ScriptStruct_Copy(lua_State *L);
int32 ScriptStruct_Compare(lua_State *L);

/**
 * Create a type interface
 */
TSharedPtr<UnLua::ITypeInterface> CreateTypeInterface(lua_State *L, int32 Index);
