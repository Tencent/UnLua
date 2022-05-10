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

#include "ContainerRegistry.h"
#include "LowLevel.h"
#include "LuaCore.h"
#include "LuaEnv.h"

namespace UnLua
{
    FContainerRegistry::FContainerRegistry(FLuaEnv* Env)
        : Env(Env)
    {
        // <FScriptArray, FLuaArray/FLuaMap/FLuaSet>
        const auto L = Env->GetMainState();
        lua_pushstring(L, "ScriptContainerMap");
        LowLevel::CreateWeakValueTable(L);
        lua_pushvalue(L, -1);
        MapRef = luaL_ref(L, -1);
        lua_rawset(L, LUA_REGISTRYINDEX);
    }

    FLuaArray* FContainerRegistry::NewArray(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaArray::EScriptArrayFlag Flag)
    {
        const FScriptArray* ScriptArray = new FScriptArray;
        void* Userdata = NewUserdata(L, FScriptContainerDesc::Array);
        const auto Ret = new(Userdata) FLuaArray(ScriptArray, ElementType, Flag);
        return Ret;
    }

    FLuaSet* FContainerRegistry::NewSet(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaSet::FScriptSetFlag Flag)
    {
        const FScriptSet* ScriptSet = new FScriptSet;
        void* Userdata = NewUserdata(L, FScriptContainerDesc::Set);
        const auto Ret = new(Userdata) FLuaSet(ScriptSet, ElementType, Flag); 
        return Ret;
    }

    FLuaMap* FContainerRegistry::NewMap(lua_State* L, TSharedPtr<ITypeInterface> KeyType, TSharedPtr<ITypeInterface> ValueType, FLuaMap::FScriptMapFlag Flag)
    {
        const FScriptMap* ScriptMap = new FScriptMap;
        void* Userdata = NewUserdata(L, FScriptContainerDesc::Map);
        const auto Ret = new(Userdata) FLuaMap(ScriptMap, KeyType, ValueType, Flag);
        return Ret;
    }

    void FContainerRegistry::FindOrAdd(lua_State* L, FScriptArray* ContainerPtr, TSharedPtr<ITypeInterface> ElementType)
    {
        void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Array);
        if (Userdata)
            new(Userdata) FLuaArray(ContainerPtr, ElementType, FLuaArray::OwnedByOther);
    }

    void FContainerRegistry::FindOrAdd(lua_State* L, FScriptSet* ContainerPtr, TSharedPtr<ITypeInterface> ElementType)
    {
        void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Set);
        if (Userdata)
            new(Userdata) FLuaSet(ContainerPtr, ElementType, FLuaSet::OwnedByOther);
    }

    void FContainerRegistry::FindOrAdd(lua_State* L, FScriptMap* ContainerPtr, TSharedPtr<ITypeInterface> KeyType, TSharedPtr<ITypeInterface> ValueType)
    {
        void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Map);
        if (Userdata)
            new(Userdata) FLuaMap(ContainerPtr, KeyType, ValueType, FLuaMap::OwnedByOther);
    }
    
    void FContainerRegistry::Remove(const FLuaArray* Container)
    {
        const auto L = Env->GetMainState();
        RemoveCachedScriptContainer(L, Container->GetContainerPtr());
    }

    void FContainerRegistry::Remove(const FLuaSet* Container)
    {
        const auto L = Env->GetMainState();
        RemoveCachedScriptContainer(L, Container->GetContainerPtr());
    }

    void FContainerRegistry::Remove(const FLuaMap* Container)
    {
        const auto L = Env->GetMainState();
        RemoveCachedScriptContainer(L, Container->GetContainerPtr());
    }

    void* FContainerRegistry::NewUserdata(lua_State* L, const FScriptContainerDesc& Desc)
    {
        void* Userdata = NewUserdataWithContainerTag(L, Desc.GetSize());
        luaL_setmetatable(L, Desc.GetName());
        return Userdata;
    }
}
