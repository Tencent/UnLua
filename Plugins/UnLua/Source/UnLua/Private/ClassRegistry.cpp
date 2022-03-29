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

#include "ClassRegistry.h"
#include "LuaEnv.h"
#include "Binding.h"
#include "LuaCore.h"
#include "UELib.h"
#include "ReflectionUtils/ClassDesc.h"
#include "ReflectionUtils/ReflectionRegistry.h"

extern int32 UObject_Identical(lua_State* L);
extern int32 UObject_Delete(lua_State* L);

static FString GetMetatableName(const UObject* Object)
{
    if (Object->IsA<UEnum>())
    {
        return Object->IsNative() ? ((UEnum*)Object)->CppType : Object->GetPathName();
    }

    const UStruct* Type = Object->IsA<UStruct>() ? (UStruct*)Object : Object->GetClass();
    if (Type->IsNative())
    {
        return FString::Printf(TEXT("%s%s"), Type->GetPrefixCPP(), *Type->GetName());
    }

    return Type->GetPathName();
}

UnLua::FClassRegistry::FClassRegistry(lua_State* GL)
    : GL(GL)
{
}

UnLua::FClassRegistry* UnLua::FClassRegistry::Find(const lua_State* L)
{
    const auto Env = FLuaEnv::FindEnv(L);
    if (Env == nullptr)
        return nullptr;
    return Env->GetClassRegistry();
}

bool UnLua::FClassRegistry::PushMetatable(lua_State* L, const char* MetatableName)
{
    int Type = luaL_getmetatable(L, MetatableName);
    if (Type == LUA_TTABLE)
        return true;
    lua_pop(L, 1);

    if (FindExportedNonReflectedClass(MetatableName))
        return false;

    FClassDesc* ClassDesc = GReflectionRegistry.RegisterClass(MetatableName);
    if (!ClassDesc)
        return false;

    luaL_newmetatable(L, MetatableName);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, Class_Index);
    lua_rawset(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, Class_NewIndex);
    lua_rawset(L, -3);

    uint64 TypeHash = (uint64)ClassDesc->AsStruct();
    lua_pushstring(L, "TypeHash");
    lua_pushnumber(L, TypeHash);
    lua_rawset(L, -3);

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();
    if (ScriptStruct)
    {
        lua_pushlightuserdata(L, ClassDesc);

        lua_pushstring(L, "Copy");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, ScriptStruct_Copy, 1);
        lua_rawset(L, -4);

        lua_pushstring(L, "CopyFrom");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, ScriptStruct_CopyFrom, 1);
        lua_rawset(L, -4);

        lua_pushstring(L, "__eq");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, ScriptStruct_Compare, 1);
        lua_rawset(L, -4);

        lua_pushstring(L, "__gc");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, ScriptStruct_Delete, 1);
        lua_rawset(L, -4);

        lua_pushstring(L, "__call");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, ScriptStruct_New, 1); // closure
        lua_rawset(L, -4);

        lua_pop(L, 1);
    }
    else
    {
        UClass* Class = ClassDesc->AsClass();
        if (Class != UObject::StaticClass() && Class != UClass::StaticClass())
        {
            lua_pushstring(L, "ClassDesc");
            lua_pushlightuserdata(L, ClassDesc);
            lua_rawset(L, -3);

            lua_pushstring(L, "StaticClass");
            lua_pushlightuserdata(L, ClassDesc);
            lua_pushcclosure(L, Class_StaticClass, 1);
            lua_rawset(L, -3);

            lua_pushstring(L, "Cast");
            lua_pushcfunction(L, Class_Cast);
            lua_rawset(L, -3);

            lua_pushstring(L, "__eq");
            lua_pushcfunction(L, UObject_Identical);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, UObject_Delete);
            lua_rawset(L, -3);
        }
    }

    lua_pushvalue(L, -1); // set metatable to self
    lua_setmetatable(L, -2);

    TArray<FClassDesc*> ClassDescChain;
    ClassDesc->GetInheritanceChain(ClassDescChain);

    TArray<IExportedClass*> ExportedClasses;
    for (int32 i = ClassDescChain.Num() - 1; i > -1; --i)
    {
        auto ExportedClass = FindExportedReflectedClass(*ClassDescChain[i]->GetName());
        if (ExportedClass)
            ExportedClass->Register(L);
    }

    UELib::SetTableForClass(L, MetatableName);

    return true;
}

bool UnLua::FClassRegistry::TrySetMetatable(lua_State* L, const char* MetatableName)
{
    if (!PushMetatable(L, MetatableName))
        return false;

    lua_setmetatable(L, -2);
    return true;
}

bool UnLua::FClassRegistry::Register(const char* MetatableName)
{
    if (!PushMetatable(GL, MetatableName))
        return false;

    lua_pop(GL, 1);
    return true;
}

bool UnLua::FClassRegistry::Register(const UStruct* Class)
{
    const auto MetatableName = GetMetatableName(Class);
    return Register(TCHAR_TO_UTF8(*MetatableName));
}
