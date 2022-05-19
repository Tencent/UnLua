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

#include "UnLuaEx.h"
#include "LuaCore.h"
#include "Containers/LuaSet.h"

static int32 TSet_New(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    TSharedPtr<UnLua::ITypeInterface> TypeInterface(CreateTypeInterface(L, 2));
    if (!TypeInterface)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to create TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
    Registry->NewSet(L, TypeInterface, FLuaSet::OwnedBySelf);
    
    return 1;
}

/**
 * @see FLuaSet::Num(...)
 */
static int32 TSet_Length(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_pushinteger(L, Set->Num());
    return 1;
}

/**
 * @see FLuaSet::Add(...)
 */
static int32 TSet_Add(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Set->ElementInterface->Initialize(Set->ElementCache);
    Set->ElementInterface->Write(L, Set->ElementCache, 2);
    Set->Add(Set->ElementCache);
    Set->ElementInterface->Destruct(Set->ElementCache);
    return 0;
}

/**
 * @see FLuaSet::Remove(...)
 */
static int32 TSet_Remove(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Set->ElementInterface->Initialize(Set->ElementCache);
    Set->ElementInterface->Write(L, Set->ElementCache, 2);
    bool bSuccess = Set->Remove(Set->ElementCache);
    Set->ElementInterface->Destruct(Set->ElementCache);
    lua_pushboolean(L, bSuccess);
    return 1;
}

/**
 * @see FLuaSet::Contains(...)
 */
static int32 TSet_Contains(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Set->ElementInterface->Initialize(Set->ElementCache);
    Set->ElementInterface->Write(L, Set->ElementCache, 2);
    bool bSuccess = Set->Contains(Set->ElementCache);
    Set->ElementInterface->Destruct(Set->ElementCache);
    lua_pushboolean(L, bSuccess);
    return 1;
}

/**
 * @see FLuaSet::Clear(...)
 */
static int32 TSet_Clear(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Set->Clear();
    return 0;
}

/**
 * @see FLuaSet::ToArray(...)
 */
static int32 TSet_ToArray(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Userdata = NewUserdataWithPadding(L, sizeof(FLuaArray), "TArray");
    FLuaArray *Array = Set->ToArray(Userdata);
    return 1;
}

/**
 * GC function
 */
static int32 TSet_Delete(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
    Registry->Remove(Set);
    
    Set->~FLuaSet();
    return 0;
}

/**
 * Convert the set to a Lua table
 */
static int32 TSet_ToTable(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaSet *Set = (FLuaSet*)(GetCppInstanceFast(L, 1));
    if (!Set)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TSet!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *MemData = FMemory::Malloc(sizeof(FLuaArray), alignof(FLuaArray));
    FLuaArray *Array = Set->ToArray(MemData);
    Array->Inner->Initialize(Array->ElementCache);
    lua_newtable(L);
    for (int32 i = 0; i < Array->Num(); ++i)
    {
        lua_pushinteger(L, i + 1);
        Array->Get(i, Array->ElementCache);
        Array->Inner->Read(L, Array->ElementCache, true);
        lua_rawset(L, -3);
    }
    Array->Inner->Destruct(Array->ElementCache);
    FMemory::Free(MemData);
    return 1;
}

static const luaL_Reg TSetLib[] =
{
    { "Length", TSet_Length },
    { "Num", TSet_Length },
    { "Add", TSet_Add },
    { "Remove", TSet_Remove },
    { "Contains", TSet_Contains },
    { "Clear", TSet_Clear },
    { "ToArray", TSet_ToArray },
    { "ToTable", TSet_ToTable },
    { "__gc", TSet_Delete },
    { "__call", TSet_New },
    { nullptr, nullptr }
};

EXPORT_UNTYPED_CLASS(TSet, false, TSetLib)
IMPLEMENT_EXPORTED_CLASS(TSet)
