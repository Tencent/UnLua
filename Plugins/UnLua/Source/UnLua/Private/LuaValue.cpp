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
    FLuaIndex::FLuaIndex(lua_State* InL, int32 InIndex)
        : L(InL), Index(LowLevel::AbsIndex(InL, InIndex))
    {
    }

    FLuaIndex::FLuaIndex(FLuaEnv* Env, int32 Index)
        : FLuaIndex(Env->GetMainState(), Index)
    {
    }

    FLuaValue::FLuaValue(lua_State* L, int32 Index)
        : FLuaIndex(L, Index), Type(lua_type(L, Index))
    {
    }

    FLuaValue::FLuaValue(lua_State* L, int32 Index, int32 Type)
        : FLuaIndex(L, Index), Type(Type)
    {
        check(lua_type(L, Index) == Type);
    }

    FLuaValue::FLuaValue(FLuaEnv* Env, int32 Index)
        : FLuaValue(Env->GetMainState(), Index)
    {
    }

    FLuaValue::FLuaValue(FLuaEnv* Env, int32 Index, int32 Type)
        : FLuaValue(Env->GetMainState(), Index, Type)
    {
    }

    FLuaTable::FLuaTable(lua_State* L, int32 Index)
        : FLuaIndex(L, Index), PushedValues(0)
    {
    }

    FLuaTable::FLuaTable(lua_State* L, FLuaValue Value)
        : FLuaIndex(L, Value.GetIndex()), PushedValues(0)
    {
    }

    FLuaTable::FLuaTable(FLuaEnv* Env, int32 Index)
        : FLuaTable(Env->GetMainState(), Index)
    {
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
            lua_pop(L, PushedValues);
            PushedValues = 0;
        }
    }

    int32 FLuaTable::Length() const
    {
        return lua_rawlen(L, Index);
    }

    FLuaValue FLuaTable::operator[](int32 i) const
    {
        lua_pushinteger(L, i);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](int64 i) const
    {
        lua_pushinteger(L, i);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](double d) const
    {
        lua_pushnumber(L, d);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](const char* s) const
    {
        lua_pushstring(L, s);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](const void* p) const
    {
        lua_pushlightuserdata(L, (void*)p);
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](FLuaIndex StackIndex) const
    {
        lua_pushvalue(L, StackIndex.GetIndex());
        lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, -1);
    }

    FLuaValue FLuaTable::operator[](FLuaValue Key) const
    {
        lua_pushvalue(L, Key.GetIndex());
        int32 Type = lua_gettable(L, Index);
        ++PushedValues;
        return FLuaValue(L, Type);
    }

    FLuaFunction::FLuaFunction(lua_State* L, const char* GlobalFuncName)
        : L(L), FunctionRef(LUA_REFNIL)
    {
        if (!GlobalFuncName)
        {
            return;
        }

        // find global function and create a reference for the function
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

    FLuaFunction::FLuaFunction(lua_State* L, const char* GlobalTableName, const char* FuncName)
        : L(L), FunctionRef(LUA_REFNIL)
    {
        if (!GlobalTableName || !FuncName)
        {
            return;
        }

        // find a function in a global table and create a reference for the function
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

    FLuaFunction::FLuaFunction(lua_State* L, FLuaTable Table, const char* FuncName)
        : L(L), FunctionRef(LUA_REFNIL)
    {
        // find a function in a table and create a reference for the function
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

    FLuaFunction::FLuaFunction(lua_State* L, FLuaValue Value)
        : L(L), FunctionRef(LUA_REFNIL)
    {
        // create a reference for the Generic Lua value if it's a function
        int32 Type = Value.GetType();
        if (Type == LUA_TFUNCTION)
        {
            lua_pushvalue(L, Value.GetIndex());
            FunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, const char* GlobalFuncName)
        : FLuaFunction(Env->GetMainState(), GlobalFuncName)
    {
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, const char* GlobalTableName, const char* FuncName)
        : FLuaFunction(Env->GetMainState(), GlobalTableName, FuncName)
    {
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, FLuaTable Table, const char* FuncName)
        : FLuaFunction(Env->GetMainState(), Table, FuncName)
    {
    }

    FLuaFunction::FLuaFunction(FLuaEnv* Env, FLuaValue Value)
        : FLuaFunction(Env->GetMainState(), Value)
    {
    }

    FLuaFunction::~FLuaFunction()
    {
        // releases reference for the function
        if (FunctionRef != LUA_REFNIL)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, FunctionRef);
        }
    }

    FLuaRetValues::FLuaRetValues(lua_State* L, int32 NumResults)
        : L(L), bValid(NumResults > -1)
    {
        if (NumResults > 0)
        {
            Values.Reserve(NumResults);
            for (int32 i = 0; i < NumResults; ++i)
            {
                Values.Add(FLuaValue(L, i - NumResults));
            }
        }
    }

    FLuaRetValues::FLuaRetValues(FLuaEnv* Env, int32 NumResults)
        : L(Env->GetMainState()), bValid(NumResults > -1)
    {
    }

    // move constructor, and disable copy constructor
    FLuaRetValues::FLuaRetValues(FLuaRetValues&& Src)
        : L(Src.L), Values(MoveTemp(Src.Values)), bValid(Src.bValid)
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
