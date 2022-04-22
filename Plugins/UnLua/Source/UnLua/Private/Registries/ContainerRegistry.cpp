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
#include "LuaCore.h"

UnLua::FContainerRegistry::FContainerRegistry(lua_State* GL)
    : GL(GL)
{
    // <FScriptArray, FLuaArray>
    lua_pushstring(GL, "ScriptContainerMap");
    CreateWeakValueTable(GL);
    lua_rawset(GL, LUA_REGISTRYINDEX);
}

void* UnLua::FContainerRegistry::New(lua_State* L, const FScriptContainerDesc& Desc)
{
    void* Userdata = NewUserdataWithContainerTag(L, Desc.GetSize());
    luaL_setmetatable(L, Desc.GetName());
    return Userdata;
}

void UnLua::FContainerRegistry::Remove(const FLuaArray* Container)
{
    RemoveCachedScriptContainer(GL, Container->GetContainerPtr());
}

void UnLua::FContainerRegistry::Remove(const FLuaSet* Container)
{
    RemoveCachedScriptContainer(GL, Container->GetContainerPtr());
}

void UnLua::FContainerRegistry::Remove(const FLuaMap* Container)
{
    RemoveCachedScriptContainer(GL, Container->GetContainerPtr());
}

FScriptArray* UnLua::FContainerRegistry::NewArray(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaArray::EScriptArrayFlag Flag)
{
    FScriptArray* ScriptArray = new FScriptArray;
    void* Userdata = NewScriptContainer(L, FScriptContainerDesc::Array);
    new(Userdata) FLuaArray(ScriptArray, ElementType, Flag);
    return ScriptArray;
}

FScriptSet* UnLua::FContainerRegistry::NewSet(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaSet::FScriptSetFlag Flag)
{
    FScriptSet* ScriptSet = new FScriptSet;
    void* Userdata = NewScriptContainer(L, FScriptContainerDesc::Set);
    new(Userdata) FLuaSet(ScriptSet, ElementType, Flag);
    return ScriptSet;
}

FScriptMap* UnLua::FContainerRegistry::NewMap(lua_State* L, TSharedPtr<ITypeInterface> KeyType, TSharedPtr<ITypeInterface> ValueType, FLuaMap::FScriptMapFlag Flag)
{
    FScriptMap* ScriptMap = new FScriptMap;
    void* Userdata = NewScriptContainer(L, FScriptContainerDesc::Map);
    new(Userdata) FLuaMap(ScriptMap, KeyType, ValueType, Flag);
    return ScriptMap;
}

void UnLua::FContainerRegistry::FindOrAdd(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FScriptArray* ContainerPtr)
{
    void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Array);
    if (Userdata)
        new(Userdata) FLuaArray(ContainerPtr, ElementType, FLuaArray::OwnedByOther);
}

void UnLua::FContainerRegistry::FindOrAdd(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FScriptSet* ContainerPtr)
{
    void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Set);
    if (Userdata)
        new(Userdata) FLuaSet(ContainerPtr, ElementType, FLuaSet::OwnedByOther);
}

void UnLua::FContainerRegistry::FindOrAdd(lua_State* L, TSharedPtr<ITypeInterface> KeyType, TSharedPtr<ITypeInterface> ValueType, FScriptMap* ContainerPtr)
{
    void* Userdata = CacheScriptContainer(L, ContainerPtr, FScriptContainerDesc::Map);
    if (Userdata)
        new(Userdata) FLuaMap(ContainerPtr, KeyType, ValueType, FLuaMap::OwnedByOther);
}
