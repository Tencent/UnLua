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
#include "ReflectionUtils/ClassDesc.h"

#if UNLUA_LEGACY_BLUEPRINT_PATH
    static void LeagcyAppendSuffix(FString& ClassPath)
    {
        const TCHAR* Suffix = TEXT("_C");
        int32 Index = INDEX_NONE;
        ClassPath.FindChar(TCHAR('.'), Index);
        if (Index == INDEX_NONE)
        {
            ClassPath.FindLastChar(TCHAR('/'), Index);
            if (Index != INDEX_NONE)
            {
                const FString Name = ClassPath.Mid(Index + 1);
                ClassPath += TCHAR('.');
                ClassPath += Name;
                ClassPath.AppendChars(Suffix, 2);
            }
        }
        else
        {
            if (ClassPath.Right(2) != TEXT("_C"))
            {
                ClassPath.AppendChars(TEXT("_C"), 2);
            }
        }
    }
#endif

/**
 * Load a class. for example: UClass.Load("/Game/Core/Blueprints/AICharacter.AICharacter_C")
 */
int32 UClass_Load(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    const char* ClassPath = lua_tostring(L, 1);
    if (!ClassPath)
        return luaL_error(L, "invalid class name");

    FString Name = UTF8_TO_TCHAR(ClassPath);

#if UNLUA_LEGACY_BLUEPRINT_PATH
    LeagcyAppendSuffix(Name);
#endif

    UClass* Class = LoadObject<UClass>(nullptr, *Name);
    if (!Class)
        return 0;

    if (!UnLua::FLuaEnv::FindEnv(L)->GetClassRegistry()->Register(Class))
        return 0;

    UnLua::PushUObject(L, Class);
    return 1;
}

/**
 * Test whether this class is a child of another class
 */
static int32 UClass_IsChildOf(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
        return luaL_error(L, "invalid parameters");

    UClass* SrcClass = Cast<UClass>(UnLua::GetUObject(L, 1));
    if (!SrcClass)
        return luaL_error(L, "invalid source class");

    UClass* TargetClass = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (!TargetClass)
        return luaL_error(L, "invalid target class");

    bool bValid = SrcClass->IsChildOf(TargetClass);
    lua_pushboolean(L, bValid);
    return 1;
}


/**
 * Get the default object of UClass.
 */
static int32 UClass_GetDefaultObject(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    UClass* SrcClass = Cast<UClass>(UnLua::GetUObject(L, 1));
    if (!SrcClass)
        return luaL_error(L, "invalid source class");

    UObject* Ret = SrcClass->GetDefaultObject();
    if (Ret)
    {
        UnLua::PushUObject(L, Ret);
    }
    else
    {
        lua_pushnil(L);
    }


    return 1;
}


static const luaL_Reg UClassLib[] =
{
    {"Load", UClass_Load},
    {"IsChildOf", UClass_IsChildOf},
    {"GetDefaultObject", UClass_GetDefaultObject},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(UClass)
    ADD_LIB(UClassLib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(UClass)
