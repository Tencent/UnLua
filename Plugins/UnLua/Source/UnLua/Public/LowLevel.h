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

namespace UnLua
{
    namespace LowLevel
    {
        FORCEINLINE int AbsIndex(lua_State* L, int Index)
        {
#if LUA_VERSION_NUM >= 502
            return lua_absindex(L, Index);
#else
            if (idx < 0 && idx > LUA_REGISTRYINDEX)
                idx += lua_gettop(L) + 1;
            return idx;
#endif
        }

        int EmptyIterator(lua_State* L);

        FORCEINLINE int PushEmptyIterator(lua_State* L)
        {
            lua_pushcfunction(L, EmptyIterator);
            return 1;
        }

        const static UObject* ReleasedPtr = (UObject*)0xDEAD;

        FORCEINLINE bool IsReleasedPtr(const void* Ptr) { return Ptr == ReleasedPtr; }

        /**
         * Create weak key table
         */
        void CreateWeakKeyTable(lua_State* L);

        /**
         * Create weak value table
         */
        void CreateWeakValueTable(lua_State* L);

        FString GetMetatableName(const UObject* Object);

        FString GetMetatableName(const UStruct* Struct);

        /* 从指定的LuaTable及其Super中找到所有Lua方法名 */
        void GetFunctionNames(lua_State* L, int TableRef, TSet<FName>& FunctionNames);

        /* Get package.loaded[ModuleName] */
        int GetLoadedModule(lua_State* L, const char* ModuleName);

        bool CheckPropertyOwner(lua_State* L, UnLua::ITypeOps* InProperty, void* InContainerPtr);

        void* GetUserdata(lua_State* L, int32 Index, bool* OutTwoLvlPtr = nullptr, bool *OutClassMetatable = nullptr);

        uint8 CalculateUserdataPadding(UStruct* Struct);
    }
}
