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

#include "ObjectRegistry.h"

#include "lauxlib.h"

static int ReleaseSharedPtr(lua_State* L)
{
    const auto Ptr = (TSharedPtr<void>*)lua_touserdata(L, 1);
    Ptr->Reset();
    return 0;
}

FObjectRegistry::FObjectRegistry(lua_State* GL)
    : GL(GL)
{
    const auto L = GL;

    luaL_newmetatable(L, "TSharedPtr");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, ReleaseSharedPtr);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

int FObjectRegistry::Ref(lua_State* L, UObject* Object, const int Index)
{
    check(!ObjectRefs.Contains(Object));
    lua_pushvalue(L, Index);
    const int Ret = luaL_ref(L, LUA_REGISTRYINDEX);
    ObjectRefs.Add(Object, Ret);
    return Ret;
}

void FObjectRegistry::NotifyUObjectDeleted(UObject* Object)
{
    int32 LuaRef;
    if (!ObjectRefs.RemoveAndCopyValue(Object, LuaRef))
        return;
    luaL_unref(GL, LUA_REGISTRYINDEX, LuaRef);
}
