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

#include "LuaValue.h"
#include "LuaEnv.h"
#include "UnLuaBase.h"

namespace UnLua
{
        /**
     * Lua stack index wrapper
     */
    FLuaIndex::FLuaIndex(FLuaEnv* Env, int32 InIndex)
        : Env(Env)
    {
        if (InIndex < 0 && InIndex > LUA_REGISTRYINDEX)
        {
            const auto L = Env->GetMainState();
            int32 Top = lua_gettop(L);
            Index = Top + InIndex + 1;
        }
        else
        {
            Index = InIndex;
        }
    }

    /**
     * Generic Lua value
     */
    FLuaValue::FLuaValue(FLuaEnv* Env, int32 Index)
        : FLuaIndex(Env, Index)
    {
        const auto L = Env->GetMainState();
        Type = lua_type(L, Index);
    }

    FLuaValue::FLuaValue(FLuaEnv* Env, int32 Index, int32 Type)
        : FLuaIndex(Env, Index), Type(Type)
    {
        const auto L = Env->GetMainState();
        check(lua_type(L, Index) == Type);
    }

    /**
     * Lua table wrapper
     */
    FLuaTable::FLuaTable(FLuaEnv* Env, int32 Index)
        : FLuaIndex(Env, Index), PushedValues(0)
    {
        const auto L = Env->GetMainState();
        check(lua_type(L, Index) == LUA_TTABLE);
    }

    FLuaTable::FLuaTable(FLuaEnv* Env, FLuaValue Value)
        : FLuaIndex(Env, Value.GetIndex()), PushedValues(0)
    {
        check(Value.GetType() == LUA_TTABLE);
    }

    FLuaTable::~FLuaTable()
    {
        Reset();
    }

    void FLuaTable::Reset()
    {
        if (PushedValues)
        {
            const auto L = Env->GetMainState();
            lua_pop(L, PushedValues);
            PushedValues = 0;
        }
    }

    int32 FLuaTable::Length() const
    {
        const auto L = Env->GetMainState();
        return lua_rawlen(L, Index);
    }

    FLuaValue FLuaTable::operator[](int32 i) const
    {
        const auto L = Env->GetMainState();
        lua_pushinteger(L, i);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](int64 i) const
    {
        const auto L = Env->GetMainState();
        lua_pushinteger(L, i);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](double d) const
    {
        const auto L = Env->GetMainState();
        lua_pushnumber(L, d);
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](const char *s) const
    {
        const auto L = Env->GetMainState();
        lua_pushstring(L, s);
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](const void *p) const
    {
        const auto L = Env->GetMainState();
        lua_pushlightuserdata(L, (void*)p);
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](FLuaIndex StackIndex) const
    {
        const auto L = Env->GetMainState();
        lua_pushvalue(L, StackIndex.GetIndex());
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, -1);
    }

    FLuaValue FLuaTable::operator[](FLuaValue Key) const
    {
        const auto L = Env->GetMainState();
        lua_pushvalue(L, Key.GetIndex());
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(Env, Type);
    }

    /**
     * Lua function wrapper
     */
    FLuaFunction::FLuaFunction(FLuaEnv* Env, const char *GlobalFuncName)
        : Env(Env), FunctionRef(LUA_REFNIL)
    {
        if (!GlobalFuncName)
        {
            return;
        }

        // find global function and create a reference for the function
        const auto L = Env->GetMainState();
        int32 Type = lua_getglobal(L, GlobalFuncName);
        if (Type == LUA_TFUNCTION)
        {
            FunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Global function %s doesn't exist!"), UTF8_TO_TCHAR(GlobalFuncName));
            lua_pop(L, 1);
        }
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, const char *GlobalTableName, const char *FuncName)
        : Env(Env), FunctionRef(LUA_REFNIL)
    {
        if (!GlobalTableName || !FuncName)
        {
            return;
        }

        // find a function in a global table and create a reference for the function
        const auto L = Env->GetMainState();
        int32 Type = lua_getglobal(L, GlobalTableName);
        if (Type == LUA_TTABLE)
        {
            Type = lua_getfield(L, -1, FuncName);
            if (Type == LUA_TFUNCTION)
            {
                FunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pop(L, 1);
            }
            else
            {
                UE_LOG(LogUnLua, Verbose, TEXT("Function %s of global table %s doesn't exist!"), UTF8_TO_TCHAR(FuncName), UTF8_TO_TCHAR(GlobalTableName));
                lua_pop(L, 2);
            }
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Global table %s doesn't exist!"), UTF8_TO_TCHAR(GlobalTableName));
            lua_pop(L, 1);
        }
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, FLuaTable Table, const char *FuncName)
        : Env(Env), FunctionRef(LUA_REFNIL)
    {
        // find a function in a table and create a reference for the function
        const auto L = Env->GetMainState();
        lua_pushstring(L, FuncName);
        int32 Type = lua_gettable(L, Table.GetIndex());
        if (Type == LUA_TFUNCTION)
        {
            FunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Function %s doesn't exist!"), UTF8_TO_TCHAR(FuncName));
            lua_pop(L, 1);
        }
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, FLuaValue Value)
        : Env(Env), FunctionRef(LUA_REFNIL)
    {
        // create a reference for the Generic Lua value if it's a function
        int32 Type = Value.GetType();
        if (Type == LUA_TFUNCTION)
        {
            const auto L = Env->GetMainState();
            lua_pushvalue(L, Value.GetIndex());
            FunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }

    FLuaFunction::~FLuaFunction()
    {
        // releases reference for the function
        if (FunctionRef != LUA_REFNIL)
        {
            const auto L = Env->GetMainState();
            luaL_unref(L, LUA_REGISTRYINDEX, FunctionRef);
        }
    }

    /**
     * Lua function return values
     */
    FLuaRetValues::FLuaRetValues(FLuaEnv* Env, int32 NumResults)
        : Env(Env), bValid(NumResults > -1)
    {
        if (NumResults > 0)
        {
            Values.Reserve(NumResults);
            for (int32 i = 0; i < NumResults; ++i)
            {
                Values.Add(FLuaValue(Env, i - NumResults));
            }
        }
    }

    // move constructor, and disable copy constructor
    FLuaRetValues::FLuaRetValues(FLuaRetValues &&Src)
        : Env(Src.Env), Values(MoveTemp(Src.Values)), bValid(Src.bValid)
    {
        check(true);
    }

    FLuaRetValues::~FLuaRetValues()
    {
        Pop();
    }

    void FLuaRetValues::Pop()
    {
        if (Values.Num() > 0)
        {
            const auto L = Env->GetMainState();
            lua_pop(L, Values.Num());
            Values.Empty();
        }
    }

    const FLuaValue& FLuaRetValues::operator[](int32 i) const
    {
        check(i > -1 && i < Values.Num());
        return Values[i];
    }
}
