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

#include "CoreUObject.h"
//#include "UnLuaCompatibility.h"
#include "lua.hpp"

#define MAX_LUA_VALUE_DEPTH 4

namespace UnLua
{

    /**
     * Helper to hold a Lua value
     */
    struct UNLUA_API FLuaDebugValue
    {
        FLuaDebugValue()
            : Depth(INDEX_NONE), bAlreadyBuilt(false)
        {}

        FString ToString() const;

        /**
         * Fill in this value
         *
         * @param Index - Lua stack index
         * @param Level - the depth of the variable's value
         */
        void Build(lua_State *L, int32 Index, int32 Level = MAX_int32);

        FString ReadableValue;
        FString Type;
        int32 Depth;
        bool bAlreadyBuilt;
        TArray<FLuaDebugValue> Keys;
        TArray<FLuaDebugValue> Values;

    private:
        FLuaDebugValue* AddKey();
        FLuaDebugValue* AddValue();

        UObjectBaseUtility* GetContainerObject(UStruct *Struct, void *ContainerPtr);
        void BuildFromUserdata(lua_State *L, int32 Index);
        void BuildFromUStruct(UStruct *Struct, void *ContainerPtr, UObjectBaseUtility *ContainerObject = nullptr);
        void BuildFromTArray(FScriptArrayHelper &ArrayHelper, const FProperty *InnerProperty);
        void BuildFromTMap(FScriptMapHelper &MapHelper);
        void BuildFromTSet(FScriptSetHelper &SetHelper);
        void BuildFromUProperty(const FProperty *Property, void *ValuePtr);
    };

    /**
     * Lua variable wrapper
     */
    struct FLuaVariable
    {
        FString Key;
        FLuaDebugValue Value;
    };


    /**
     * Get local variables and upvalues of the runtime stack
     *
     * @param StackLevel - stack level. level 0 is the current running function, whereas level n+1 is the function that has called level n
     * @param[out] LocalVariables - local variables
     * @param[out] Upvalues - upvalues
     * @param Level - the depth of the variable's value
     * @return - true if successful, false otherwise
     */
    UNLUA_API bool GetStackVariables(lua_State *L, int32 StackLevel, TArray<FLuaVariable> &LocalVariables, TArray<FLuaVariable> &Upvalues, int32 Level = MAX_int32);

    /**
     * Get call stack
     *
     * @return - full description
     */
    UNLUA_API FString GetLuaCallStack(lua_State *L);

    /* 在IDE断点调试窗口中直接运行UnLua::PrintCallStack(L)来打印当前堆栈 */
    void PrintCallStack(lua_State* L);
} // namespace UnLua
