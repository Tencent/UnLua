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

#include "lauxlib.h"
#include "UELib.h"

#include "Binding.h"
#include "Registries/ClassRegistry.h"
#include "LuaCore.h"
#include "LuaEnv.h"
#include "Registries/EnumRegistry.h"

static const char* REGISTRY_KEY = "UnLua_UELib";
static const char* NAMESPACE_NAME = "UE";

static void LoadUEType(const char* InName)
{
    FString Name = UTF8_TO_TCHAR(InName);

    // find candidates in memory
    UField* Ret = FindObject<UClass>(ANY_PACKAGE, *Name);
    if (!Ret)
        Ret = FindObject<UScriptStruct>(ANY_PACKAGE, *Name);
    if (!Ret)
        Ret = FindObject<UEnum>(ANY_PACKAGE, *Name);

    // load candidates if not found
    if (!Ret)
        Ret = LoadObject<UClass>(nullptr, *Name);
    if (!Ret)
        Ret = LoadObject<UScriptStruct>(nullptr, *Name);
    if (!Ret)
        Ret = LoadObject<UEnum>(nullptr, *Name);

    // TODO: if (!Ret->IsNative())
}

static int UE_Index(lua_State* L)
{
    const int32 Type = lua_type(L, 2);
    if (Type != LUA_TSTRING)
        return 0;

    const char* Name = lua_tostring(L, 2);
    const auto Exported = UnLua::FindExportedNonReflectedClass(Name);
    if (Exported)
    {
        Exported->Register(L);
        lua_rawget(L, 1);
        return 1;
    }

    const char Prefix = Name[0];
    const auto& Env = UnLua::FLuaEnv::FindEnvChecked(L);
    if (Prefix == 'U' || Prefix == 'A' || Prefix == 'F')
    {
        const auto ReflectedType = UnLua::FClassRegistry::LoadReflectedType(Name + 1);
        if (!ReflectedType)
            return 0;

        if (ReflectedType->IsNative())
        {
            if (auto Struct = Cast<UStruct>(ReflectedType))
                Env.GetClassRegistry()->Register(Struct);
        }
        else
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to load a blueprint type %s with UE namespace, use UE.UClass.Load or UE.UObject.Load instead."), UTF8_TO_TCHAR(Name));
            return 0;
        }
    }
    else if (Prefix == 'E')
    {
        const auto ReflectedType = UnLua::FClassRegistry::LoadReflectedType(Name);
        if (!ReflectedType)
            return 0;

        if (ReflectedType->IsNative())
        {
            if (auto Enum = Cast<UEnum>(ReflectedType))
                Env.GetEnumRegistry()->Register(Enum);
        }
        else
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to load a blueprint enum %s with UE namespace, use UE.UObject.Load instead."), UTF8_TO_TCHAR(Name));
            return 0;
        }
    }

    lua_rawget(L, 1);
    return 1;
}

int UnLua::UELib::Open(lua_State* L)
{
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, UE_Index);
    lua_rawset(L, -3);

    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_pushstring(L, REGISTRY_KEY);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_setglobal(L, NAMESPACE_NAME);

#if WITH_UE4_NAMESPACE == 1
    // 兼容UE4访问
    lua_getglobal(L, NAMESPACE_NAME);
    lua_setglobal(L, "UE4");
#elif WITH_UE4_NAMESPACE == 0
    // 兼容无UE4全局访问
    lua_getglobal(L, "_G");
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_getglobal(L, NAMESPACE_NAME);
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
#endif

    return 1;
}

void UnLua::UELib::SetTableForClass(lua_State* L, const char* Name)
{
    lua_getglobal(L, NAMESPACE_NAME);
    lua_pushstring(L, Name);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}
