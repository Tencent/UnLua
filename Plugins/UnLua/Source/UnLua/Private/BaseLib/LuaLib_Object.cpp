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
#include "UEReflectionUtils.h"
#include "UEObjectReferencer.h"
#include "DelegateHelper.h"

/**
 * Load an object. for example: UObject.Load("/Game/Core/Blueprints/AI/BehaviorTree_Enemy.BehaviorTree_Enemy")
 * @see LoadObject(...)
 */
int32 UObject_Load(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char *ObjectName = lua_tostring(L, 1);
    if (!ObjectName)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid class name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FString ObjectPath(ObjectName);
    int32 Index = INDEX_NONE;
    ObjectPath.FindChar(TCHAR('.'), Index);
    if (Index == INDEX_NONE)
    {
        ObjectPath.FindLastChar(TCHAR('/'), Index);
        if (Index != INDEX_NONE)
        {
            const FString Name = ObjectPath.Mid(Index + 1);
            ObjectPath += TCHAR('.');
            ObjectPath += Name;
        }
    }

    UObject *Object = LoadObject<UObject>(nullptr, *ObjectPath);
    if (Object)
    {
        UnLua::PushUObject(L, Object);
    }
    else
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Failed to load object %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(ObjectName));
        lua_pushnil(L);
    }

    return 1;
}

/**
 * Test validity of an object
 */
static int32 UObject_IsValid(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    bool bValid = ::IsValid(Object);
    lua_pushboolean(L, bValid);
    return 1;
}

/**
 * Get the name of an object (with no path information)
 */
static int32 UObject_GetName(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FString Name = Object->GetName();
    lua_pushstring(L, TCHAR_TO_ANSI(*Name));
    return 1;
}

/**
 * Get the UObject this object resides in
 */
static int32 UObject_GetOuter(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Outer = Object->GetOuter();
    UnLua::PushUObject(L, Outer);
    return 1;
}

/**
 * Get the UClass that defines the fields of this object
 */
static int32 UObject_GetClass(lua_State *L)
{
    UClass *Class = nullptr;
    int32 NumParams = lua_gettop(L);
    if (NumParams > 0)
    {
        UObject *Object = UnLua::GetUObject(L, 1);
        if (!Object)
        {
            UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }
        Class = Object->GetClass();
    }
    else
    {
        Class = UObject::StaticClass();
    }
    UnLua::PushUObject(L, Class);
    return 1;
}

/**
 * Get the UWorld this object is contained within
 */
static int32 UObject_GetWorld(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    class UWorld *World = Object->GetWorld();
    UnLua::PushUObject(L, (UObject*)World, false);        // GWorld will be added to Root, so we don't collect it...
    return 1;
}

/**
 * Test whether this object is of the specified type
 */
static int32 UObject_IsA(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (!Class)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid class!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    bool bValid = Object->IsA(Class);
    lua_pushboolean(L, bValid);
    return 1;
}

/**
 * Test whether two objects are identical
 */
int32 UObject_Identical(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *A = UnLua::GetUObject(L, 1);
    UObject *B = UnLua::GetUObject(L, 2);
    lua_pushboolean(L, A == B);
    return 1;
}

/**
 * GC function
 */
int32 UObject_Delete(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Object = nullptr;
    bool bTwoLvlPtr = false;
    bool bClassMetatable = false;
    void *Userdata = GetUserdata(L, 1, &bTwoLvlPtr, &bClassMetatable);
    if (Userdata)
    {
        Object = bTwoLvlPtr ? (UObject*)(*((void**)Userdata)) : (UObject*)Userdata;
    }
    else
    {
        return 0;
    }

    if (bClassMetatable)
    {
        FClassDesc *Class = (FClassDesc*)Userdata;
        if (Class->IsValid())
        {
            int32 Type = luaL_getmetatable(L, Class->GetAnsiName());
            lua_pop(L, 1);
            if (Type == LUA_TTABLE)        // meta table is registered again
            {
                if (Class->GetRefCount() > 1)
                {
                    Class->Release();
                }
                return 0;
            }
            else
            {
                if (Class->GetRefCount() == 1)
                {
                    FDelegateHelper::CleanUpByClass(Class->AsClass());
                }
            }
        }

        GReflectionRegistry.UnRegisterClass(Class);
    }

    GObjectReferencer.RemoveObjectRef(Object);

    return 0;
}

/**
 * Glue functions for UObject
 */
static const luaL_Reg UObjectLib[] =
{
    { "Load", UObject_Load },
    { "IsValid", UObject_IsValid },
    { "GetName", UObject_GetName },
    { "GetOuter", UObject_GetOuter },
    { "GetClass", UObject_GetClass },
    { "GetWorld", UObject_GetWorld },
    { "IsA", UObject_IsA },
    { "__eq", UObject_Identical },
    { "__gc", UObject_Delete },
    { nullptr, nullptr }
};

/**
 * Export UObject
 */
BEGIN_EXPORT_REFLECTED_CLASS(UObject)
    ADD_LIB(UObjectLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(UObject)

/**
 * Export FSoftObjectPtr
 */
BEGIN_EXPORT_CLASS(FSoftObjectPtr, const UObject*)
    ADD_CONST_FUNCTION_EX("IsValid", bool, IsValid)
    ADD_FUNCTION_EX("Reset", void, Reset)
    ADD_FUNCTION_EX("Set", void, operator=, const UObject*)
    ADD_CONST_FUNCTION_EX("Get", UObject*, Get)
    ADD_NAMED_FUNCTION("__tostring", ToString)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FSoftObjectPtr)

/**
 * Export FLazyObjectPtr
 */
BEGIN_EXPORT_CLASS(FLazyObjectPtr, const UObject*)
    ADD_CONST_FUNCTION_EX("IsValid", bool, IsValid)
    ADD_FUNCTION_EX("Reset", void, Reset)
    ADD_FUNCTION_EX("Set", void, operator=, const UObject*)
    ADD_CONST_FUNCTION_EX("Get", UObject*, Get)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FLazyObjectPtr)

/**
 * Export FWeakObjectPtr
 */
BEGIN_EXPORT_CLASS(FWeakObjectPtr, const UObject*)
    ADD_FUNCTION(Reset)
    ADD_FUNCTION_EX("Set", void, operator=, const UObject*)
    ADD_CONST_FUNCTION_EX("Get", UObject*, Get)
    ADD_CONST_FUNCTION_EX("IsValid", bool, IsValid)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FWeakObjectPtr)
