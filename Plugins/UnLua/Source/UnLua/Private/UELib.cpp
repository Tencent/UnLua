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

#include "UELib.h"
#include "Binding.h"
#include "Registries/ClassRegistry.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "LuaEnv.h"
#include "Registries/EnumRegistry.h"

static const char* REGISTRY_KEY = "UnLua_UELib";
static const char* NAMESPACE_NAME = "UE";

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

extern int32 UObject_Load(lua_State *L);
extern int32 UClass_Load(lua_State *L);
static int32 Global_NewObject(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 1));
    if (!Class)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid class!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject* Outer = UnLua::GetUObject(L, 2);
    if (!Outer)
        Outer = GetTransientPackage();

    FName Name = NumParams > 2 ? FName(lua_tostring(L, 3)) : NAME_None;
    //EObjectFlags Flags = NumParams > 3 ? EObjectFlags(lua_tointeger(L, 4)) : RF_NoFlags;

    {
        const char *ModuleName = NumParams > 3 ? lua_tostring(L, 4) : nullptr;
        int32 TableRef = LUA_NOREF;
        if (NumParams > 4 && lua_type(L, 5) == LUA_TTABLE)
        {
            lua_pushvalue(L, 5);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 26
        UObject* Object = StaticConstructObject_Internal(Class, Outer, Name);
#else
        FStaticConstructObjectParameters ObjParams(Class);
        ObjParams.Outer = Outer;
        ObjParams.Name = Name;
        UObject* Object = StaticConstructObject_Internal(ObjParams);
#endif
        if (Object)
        {
            UnLua::PushUObject(L, Object);
        }
        else
        {
            UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to new object for class %s!"), ANSI_TO_TCHAR(__FUNCTION__), *Class->GetName());
            return 0;
        }
    }

    return 1;
}

static constexpr luaL_Reg UE_Functions[] = {
    {"LoadObject", UObject_Load},
    {"LoadClass", UClass_Load},
    {"NewObject", Global_NewObject},
    {NULL, NULL}
};

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

    luaL_setfuncs(L, UE_Functions, 0);
    lua_setglobal(L, NAMESPACE_NAME);

    // global access for legacy support
    lua_getglobal(L, LUA_GNAME);
    luaL_setfuncs(L, UE_Functions, 0);
    lua_pop(L, 1);

#if WITH_UE4_NAMESPACE == 1
    // 兼容UE4访问
    lua_getglobal(L, NAMESPACE_NAME);
    lua_setglobal(L, "UE4");
#elif WITH_UE4_NAMESPACE == 0
    // 兼容无UE4全局访问
    lua_getglobal(L, LUA_GNAME);
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
