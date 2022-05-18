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
#include "LowLevel.h"
#include "LuaEnv.h"
#include "UnLuaDelegates.h"

namespace UnLua
{
    static const char* ObjectMap = "ObjectMap";

    static int ReleaseSharedPtr(lua_State* L)
    {
        const auto Ptr = (TSharedPtr<void>*)lua_touserdata(L, 1);
        Ptr->Reset();
        return 0;
    }

    FObjectRegistry::FObjectRegistry(FLuaEnv* Env)
        : Env(Env)
    {
        const auto L = Env->GetMainState();

        lua_pushstring(L, ObjectMap); // create weak table 'ObjectMap'
        LowLevel::CreateWeakValueTable(L);
        lua_rawset(L, LUA_REGISTRYINDEX);

        luaL_newmetatable(L, "TSharedPtr");
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, ReleaseSharedPtr);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    void FObjectRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        Unbind(Object);
    }

    void FObjectRegistry::NotifyUObjectLuaGC(UObject* Object)
    {
        Env->AutoObjectReference.Remove(Object);
    }

    void FObjectRegistry::Push(lua_State* L, UObject* Object)
    {
        if (!Object)
        {
            lua_pushnil(L);
            return;
        }

        lua_getfield(L, LUA_REGISTRYINDEX, ObjectMap);
        lua_pushlightuserdata(L, Object);
        const auto Type = lua_rawget(L, -2);
        if (Type == LUA_TNIL)
        {
            lua_pop(L, 1);
            PushObjectCore(L, Object);
            lua_pushlightuserdata(L, Object);
            lua_pushvalue(L, -2);
            lua_rawset(L, -4);

            if (!Object->IsNative())
                Env->AutoObjectReference.Add(Object);

            ObjectRefs.Add(Object, LUA_NOREF);
        }
        lua_remove(L, -2);
    }

    int FObjectRegistry::Bind(UObject* Object, const char* ModuleName)
    {
        // TODO: remove dependency of module name
        if (const auto Exists = ObjectRefs.Find(Object))
        {
            if (*Exists != LUA_NOREF)
                return *Exists;
        }

        const auto L = Env->GetMainState();

        int OldTop = lua_gettop(L);

        lua_getfield(L, LUA_REGISTRYINDEX, ObjectMap);
        lua_pushlightuserdata(L, Object);
        lua_newtable(L); // create a Lua table ('INSTANCE')
        PushObjectCore(L, Object); // push UObject ('RAW_UOBJECT')
        lua_pushstring(L, "Object");
        lua_pushvalue(L, -2);
        lua_rawset(L, -4); // INSTANCE.Object = RAW_UOBJECT

        // in some case may occur module or object metatable can 
        // not be found problem
        int32 TypeModule = GetLoadedModule(L, ModuleName); // push the required module/table ('REQUIRED_MODULE') to the top of the stack
        int32 TypeMetatable = lua_getmetatable(L, -2); // get the metatable ('METATABLE_UOBJECT') of 'RAW_UOBJECT' 
        if ((TypeModule != LUA_TTABLE)
            || (0 == TypeMetatable))
        {
            lua_pop(L, lua_gettop(L) - OldTop);
            return LUA_REFNIL;
        }

#if ENABLE_CALL_OVERRIDDEN_FUNCTION
        lua_pushstring(L, "Overridden");
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
#endif
        lua_setmetatable(L, -2); // REQUIRED_MODULE.metatable = METATABLE_UOBJECT
        lua_setmetatable(L, -3); // INSTANCE.metatable = REQUIRED_MODULE
        lua_pop(L, 1);

        lua_pushvalue(L, -1);
        const auto Ret = luaL_ref(L, LUA_REGISTRYINDEX);
        ObjectRefs.Add(Object, Ret);

        FUnLuaDelegates::OnObjectBinded.Broadcast(Object); // 'INSTANCE' is on the top of stack now

        lua_rawset(L, -3);
        lua_pop(L, 1);
        return Ret;
    }

    bool FObjectRegistry::IsBound(const UObject* Object) const
    {
        const auto Exists = ObjectRefs.Find(Object);
        return Exists && *Exists != LUA_NOREF;
    }

    int FObjectRegistry::GetBoundRef(const UObject* Object) const
    {
        const auto Ref = ObjectRefs.Find(Object);
        if (Ref)
            return *Ref;
        return LUA_NOREF;
    }

    void FObjectRegistry::Unbind(UObject* Object)
    {
        int32 Ref;
        if (!ObjectRefs.RemoveAndCopyValue(Object, Ref))
            return;

        RemoveFromObjectMapAndPushToStack(Object);
        const auto L = Env->GetMainState();
        if (Ref == LUA_NOREF)
        {
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);
                return;
            }
            check(lua_isuserdata(L, -1));
            bool bTwoLvlPtr;
            void* Userdata = GetUserdataFast(L, -1, &bTwoLvlPtr);
            check(bTwoLvlPtr)
            *((void**)Userdata) = (void*)LowLevel::ReleasedPtr;
            lua_pop(L, 1);
            return;
        }

        check(lua_istable(L, -1));
        luaL_unref(L, LUA_REGISTRYINDEX, Ref);
        FUnLuaDelegates::OnObjectUnbinded.Broadcast(Object); // object instance ('INSTANCE') is on the top of stack now

        lua_pushstring(L, "Object");
        lua_rawget(L, -2);
        void* Userdata = lua_touserdata(L, -1);
        *((void**)Userdata) = (void*)LowLevel::ReleasedPtr;
        lua_pop(L, 1);

        // INSTANCE.Object = nil
        lua_pushlightuserdata(L, Object);
        lua_pushnil(L);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }

    void FObjectRegistry::RemoveFromObjectMapAndPushToStack(UObject* Object)
    {
        const auto L = Env->GetMainState();
        lua_getfield(L, LUA_REGISTRYINDEX, ObjectMap);
        lua_pushlightuserdata(L, Object);
        lua_rawget(L, -2);
        lua_pushlightuserdata(L, Object);
        lua_pushnil(L);
        lua_rawset(L, -4);
        lua_remove(L, -2);
    }
}
