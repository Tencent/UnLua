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

#include "lua.h"
#include "LuaCore.h"
#include "Containers/LuaArray.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"

namespace UnLua
{
    class FContainerRegistry
    {
    public:
        explicit FContainerRegistry(lua_State* GL);

        /**
         * Create a new userdata for a script container
         */
        static void* New(lua_State* L, const FScriptContainerDesc& Desc);

        // TODO: refactor
        void Remove(const FLuaArray* Container);

        // TODO: refactor
        void Remove(const FLuaSet* Container);

        // TODO: refactor
        void Remove(const FLuaMap* Container);

        // TODO: refactor
        FScriptArray* NewArray(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaArray::EScriptArrayFlag Flag);

        // TODO: refactor
        void NewSet(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FLuaSet::FScriptSetFlag Flag);

        // TODO: refactor
        void NewMap(lua_State* L, TSharedPtr<ITypeInterface> KeyType, TSharedPtr<ITypeInterface> ValueType, FLuaMap::FScriptMapFlag Flag);

        // TODO: refactor
        void FindOrAdd(lua_State* L, TSharedPtr<ITypeInterface> ElementType, FScriptArray* ContainerPtr, bool bByVal);

        lua_State* GL;
    };
}
