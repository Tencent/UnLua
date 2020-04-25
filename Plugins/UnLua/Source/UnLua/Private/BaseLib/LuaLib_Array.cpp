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
#include "Containers/LuaArray.h"

static int32 TArray_New(lua_State *L)
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
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to create TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FScriptArray *ScriptArray = new FScriptArray;
    void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Array);
    new(Userdata) FLuaArray(ScriptArray, TypeInterface, FLuaArray::OwnedBySelf);
    return 1;
}

/**
 * @see FLuaArray::Num(...)
 */
static int32 TArray_Length(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_pushinteger(L, Array->Num());
    return 1;
}

/**
 * @see FLuaArray::Add(...)
 */
static int32 TArray_Add(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = Array->AddDefaulted();
    uint8 *Data = Array->GetData(Index);
    Array->Inner->Write(L, Data, 2);
    ++Index;
    lua_pushinteger(L, Index);
    return 1;
}

/**
 * @see FLuaArray::AddUnique(...)
 */
static int32 TArray_AddUnique(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 2);
    int32 Index = Array->AddUnique(Array->ElementCache);
    ++Index;
    lua_pushinteger(L, Index);
    return 1;
}

/**
 * @see FLuaArray::Find(...)
 */
static int32 TArray_Find(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 2);
    int32 Index = Array->Find(Array->ElementCache);
    ++Index;
    lua_pushinteger(L, Index);
    return 1;
}

/**
 * @see FLuaArray::Insert(...)
 */
static int32 TArray_Insert(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 2);
    int32 Index = lua_tointeger(L, 3);
    --Index;
    Array->Insert(Array->ElementCache, Index);
    return 0;
}

/**
 * @see FLuaArray::Remove(...)
 */
static int32 TArray_Remove(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = lua_tointeger(L, 2);
    --Index;
    Array->Remove(Index);
    return 0;
}

/**
 * @see FLuaArray::RemoveItem(...)
 */
static int32 TArray_RemoveItem(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 2);
    int32 N = Array->RemoveItem(Array->ElementCache);
    lua_pushinteger(L, N);
    return 1;
}

/**
 * @see FLuaArray::Clear(...)
 */
static int32 TArray_Clear(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Clear();
    return 0;
}

/**
 * @see FLuaArray::Reserve(...)
 */
static int32 TArray_Reserve(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Size = lua_tointeger(L, 2);
    bool bSuccess = Array->Reserve(Size);
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: 'Reserve' is only valid for empty TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
    lua_pushboolean(L, bSuccess);
    return 1;
}

/**
 * @see FLuaArray::Resize(...)
 */
static int32 TArray_Resize(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 NewSize = lua_tointeger(L, 2);
    Array->Resize(NewSize);
    return 0;
}

/**
 * @see FLuaArray::GetData(...)
 */
static int32 TArray_GetData(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Data = Array->GetData();
    lua_pushlightuserdata(L, Data);
    return 1;
}

/**
 * @see FLuaArray::Get(...). Create a copy for the element
 */
static int32 TArray_Get(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = lua_tointeger(L, 2);
    --Index;
    if (!Array->IsValidIndex(Index))
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: TArray Invalid Index!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    Array->Inner->Initialize(Array->ElementCache);
    Array->Get(Index, Array->ElementCache);
    Array->Inner->Read(L, Array->ElementCache, true);
    return 1;
}

/**
 * @see FLuaArray::Get(...). Return a reference for the element
 */
static int32 TArray_GetRef(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = lua_tointeger(L, 2);
    --Index;
    const void *Element = Array->GetData(Index);
    Array->Inner->Read(L, Element, false);
    return 1;
}

/**
 * @see FLuaArray::Set(...)
 */
static int32 TArray_Set(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = lua_tointeger(L, 2);
    --Index;
    if (!Array->IsValidIndex(Index))
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: TArray Invalid Index!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 3);
    Array->Set(Index, Array->ElementCache);
    return 0;
}

/**
 * @see FLuaArray::Swap(...)
 */
static int32 TArray_Swap(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 A = lua_tointeger(L, 2);
    int32 B = lua_tointeger(L, 3);
    --A;
    --B;
    Array->Swap(A, B);
    return 0;
}

/**
 * @see FLuaArray::Shuffle(...)
 */
static int32 TArray_Shuffle(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Shuffle();
    return 0;
}

/**
 * Get the last index of the array
 */
static int32 TArray_LastIndex(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = Array->Num();
    lua_pushinteger(L, Index);
    return 1;
}

/**
 * @see FLuaArray::IsValidIndex(...)
 */
static int32 TArray_IsValidIndex(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Index = lua_tointeger(L, 2);
    --Index;
    bool bValid = Array->IsValidIndex(Index);
    lua_pushboolean(L, bValid);
    return 1;
}

/**
 * @see FLuaArray::Find(...)
 */
static int32 TArray_Contains(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Inner->Initialize(Array->ElementCache);
    Array->Inner->Write(L, Array->ElementCache, 2);
    int32 N = Array->Num();
    int32 Index = Array->Find(Array->ElementCache);
    bool bResult = Index > INDEX_NONE && Index < N;
    lua_pushboolean(L, bResult);
    return 1;
}

/**
 * @see FLuaArray::Append(...)
 */
static int32 TArray_Append(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *SourceArray = (FLuaArray*)(GetCppInstanceFast(L, 2));
    if (!SourceArray)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid source TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->Append(*SourceArray);
    return 0;
}

/**
 * GC function
 */
static int32 TArray_Delete(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    Array->~FLuaArray();
    return 0;
}

/**
 * Convert the array to a Lua table
 */
static int32 TArray_ToTable(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FLuaArray *Array = (FLuaArray*)(GetCppInstanceFast(L, 1));
    if (!Array)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_newtable(L);
    for (int32 i = 0; i < Array->Num(); ++i)
    {
        lua_pushinteger(L, i + 1);
        Array->Inner->Initialize(Array->ElementCache);
        Array->Get(i, Array->ElementCache);
        Array->Inner->Read(L, Array->ElementCache, true);
        lua_rawset(L, -3);
    }
    return 1;
}

static const luaL_Reg TArrayLib[] =
{
    { "Length", TArray_Length },
    { "Add", TArray_Add },
    { "AddUnique", TArray_AddUnique },
    { "Find", TArray_Find },
    { "Insert", TArray_Insert },
    { "Remove", TArray_Remove },
    { "RemoveItem", TArray_RemoveItem },
    { "Clear", TArray_Clear },
    { "Reserve", TArray_Reserve },
    { "Resize", TArray_Resize },
    { "GetData", TArray_GetData },
    { "Get", TArray_Get },
    { "GetRef", TArray_GetRef },
    { "Set", TArray_Set },
    { "Swap", TArray_Swap },
    { "Shuffle", TArray_Shuffle },
    { "LastIndex", TArray_LastIndex },
    { "IsValidIndex", TArray_IsValidIndex },
    { "Contains", TArray_Contains },
    { "Append", TArray_Append },
    { "ToTable", TArray_ToTable },
    { "__gc", TArray_Delete },
    { "__call", TArray_New },
    { nullptr, nullptr }
};

EXPORT_UNTYPED_CLASS(TArray, false, TArrayLib)
IMPLEMENT_EXPORTED_CLASS(TArray)
