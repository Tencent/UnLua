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

namespace UnLua
{
    class FLuaEnv;

    /**
     * Lua stack index wrapper
     */
    struct UNLUA_API FLuaIndex
    {
        FLuaIndex(lua_State* L, int32 Index);
        FLuaIndex(FLuaEnv* Env, int32 Index);

        FORCEINLINE int32 GetIndex() const { return Index; }

    protected:
        lua_State* L;
        int32 Index;
    };

    /**
     * Generic Lua value
     */
    struct UNLUA_API FLuaValue : public FLuaIndex
    {
        explicit FLuaValue(lua_State* L, int32 Index);
        explicit FLuaValue(lua_State* L, int32 Index, int32 Type);
        explicit FLuaValue(FLuaEnv* Env, int32 Index);
        explicit FLuaValue(FLuaEnv* Env, int32 Index, int32 Type);

        FORCEINLINE int32 GetType() const { return Type; }

        template <typename T>
        T Value() const;

        template <typename T>
        operator T() const;

        template <typename T, typename Allocator>
        operator TArray<T, Allocator>&() const;

        template <typename T, typename KeyFunc, typename Allocator>
        operator TSet<T, KeyFunc, Allocator>&() const;

        template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
        operator TMap<KeyType, ValueType, Allocator, KeyFunc>&() const;

    private:
        int32 Type;
    };

    /**
     * Lua function return values
     */
    struct UNLUA_API FLuaRetValues
    {
        explicit FLuaRetValues(lua_State* L, int32 NumResults);
        explicit FLuaRetValues(FLuaEnv* Env, int32 NumResults);
        FLuaRetValues(FLuaRetValues&& Src);
        ~FLuaRetValues();

        void Pop();
        FORCEINLINE bool IsValid() const { return bValid; }
        FORCEINLINE int32 Num() const { return Values.Num(); }

        const FLuaValue& operator[](int32 i) const;

    private:
        FLuaRetValues(const FLuaRetValues& Src);

        lua_State* L;
        TArray<FLuaValue> Values;
        bool bValid;
    };

    /**
     * Lua table wrapper
     */
    struct UNLUA_API FLuaTable : public FLuaIndex
    {
        explicit FLuaTable(lua_State* L, int32 Index);
        explicit FLuaTable(lua_State* L, FLuaValue Value);
        explicit FLuaTable(FLuaEnv* Env, int32 Index);
        explicit FLuaTable(FLuaEnv* Env, FLuaValue Value);
        ~FLuaTable();

        void Reset();
        int32 Length() const;

        FLuaValue operator[](int32 i) const;
        FLuaValue operator[](int64 i) const;
        FLuaValue operator[](double d) const;
        FLuaValue operator[](const char* s) const;
        FLuaValue operator[](const void* p) const;
        FLuaValue operator[](FLuaIndex StackIndex) const;
        FLuaValue operator[](FLuaValue Key) const;

        template <typename... T>
        FLuaRetValues Call(const char* FuncName, T&&... Args) const;

    private:
        mutable int32 PushedValues;
    };

    /**
     * Lua function wrapper
     */
    struct UNLUA_API FLuaFunction
    {
        explicit FLuaFunction(lua_State* L, const char* GlobalFuncName);
        explicit FLuaFunction(lua_State* L, const char* GlobalTableName, const char* FuncName);
        explicit FLuaFunction(lua_State* L, FLuaTable Table, const char* FuncName);
        explicit FLuaFunction(lua_State* L, FLuaValue Value);
        explicit FLuaFunction(FLuaEnv* Env, const char* GlobalFuncName);
        explicit FLuaFunction(FLuaEnv* Env, const char* GlobalTableName, const char* FuncName);
        explicit FLuaFunction(FLuaEnv* Env, FLuaTable Table, const char* FuncName);
        explicit FLuaFunction(FLuaEnv* Env, FLuaValue Value);
        ~FLuaFunction();

        FORCEINLINE bool IsValid() const { return FunctionRef != LUA_REFNIL; }

        template <typename... T>
        FLuaRetValues Call(T&&... Args) const;

    private:
        lua_State* L;
        int32 FunctionRef;
    };
}
