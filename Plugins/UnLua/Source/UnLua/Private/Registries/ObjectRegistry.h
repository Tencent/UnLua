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

#pragma once

#include "lua.hpp"
#include "UnLuaBase.h"
#include "ReflectionUtils/FunctionDesc.h"

namespace UnLua
{
    class FLuaEnv;

    struct FManualRefProxy
    {
        TWeakObjectPtr<UObject> Object;
    };

    class FObjectRegistry
    {
    public:
        explicit FObjectRegistry(FLuaEnv* Env);

        void NotifyUObjectDeleted(UObject* Object);

        void NotifyUObjectLuaGC(UObject* Object);

        template <typename T>
        void Push(lua_State* L, TSharedPtr<T> Ptr);

        void Push(lua_State* L, UObject* Object);

        template <typename T>
        FORCEINLINE TSharedPtr<T> Get(lua_State* L, int Index);

        /**
         * 将一个UObject绑定到Lua环境，作为lua table访问。
         * @return lua引用ID
         */
        int Bind(UObject* Object);

        /**
         * 获取一个值，表示UObject是否绑定到了Lua环境。
         */
        bool IsBound(const UObject* Object) const;

        /**
         * 获取指定UObject在Lua里绑定的table的引用ID。
         * @return 若没有绑定过则返回LUA_NOREF。
         */
        int GetBoundRef(const UObject* Object) const;

        /**
         * 将指定的UObject从Lua环境解绑。
         */
        void Unbind(UObject* Object);

        /**
         * 增加对指定对象的手动引用，并将对应的代理对象压入栈顶
         */;
        void AddManualRef(lua_State* L, UObject* Object);

        /**
         * 强制移除指定对象的手动引用
         */
        void RemoveManualRef(UObject* Object);

    private:
        void RemoveFromObjectMapAndPushToStack(UObject* Object);

        FLuaEnv* Env;
        TMap<UObject*, int32> ObjectRefs;
    };

    template <typename T>
    void FObjectRegistry::Push(lua_State* L, TSharedPtr<T> Ptr)
    {
        const auto Userdata = lua_newuserdata(L, sizeof(TSharedPtr<T>));
        luaL_getmetatable(L, "TSharedPtr");
        lua_setmetatable(L, -2);
        new(Userdata) TSharedPtr<T>(Ptr);
    }

    template <typename T>
    TSharedPtr<T> FObjectRegistry::Get(lua_State* L, int Index)
    {
        const auto Ptr = lua_touserdata(L, Index);
        return *static_cast<TSharedPtr<T>*>(Ptr);
    }
}
