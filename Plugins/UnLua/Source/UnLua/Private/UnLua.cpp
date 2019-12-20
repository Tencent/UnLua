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

#include "UnLuaEx.h"
#include "UnLuaPrivate.h"
#include "UnLuaDelegates.h"
#include "LuaContext.h"

DEFINE_LOG_CATEGORY(LogUnLua);

DEFINE_STAT(STAT_UnLua_Lua_Memory);
DEFINE_STAT(STAT_UnLua_PersistentParamBuffer_Memory);
DEFINE_STAT(STAT_UnLua_OutParmRec_Memory);

namespace UnLua
{

    bool AddTypeInterface(FName Name, TSharedPtr<ITypeInterface> TypeInterface)
    {
        FLuaContext::Create();
        return GLuaCxt->AddTypeInterface(Name, TypeInterface);
    }

    IExportedClass* FindExportedClass(FName Name)
    {
        FLuaContext::Create();
        return GLuaCxt->FindExportedClass(Name);
    }

    bool ExportClass(IExportedClass *Class)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportClass(Class);
    }

    bool ExportFunction(IExportedFunction *Function)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportFunction(Function);
    }

    bool ExportEnum(IExportedEnum *Enum)
    {
        FLuaContext::Create();
        return GLuaCxt->ExportEnum(Enum);
    }

    lua_State* CreateState()
    {
        if (GLuaCxt)
        {
            GLuaCxt->CreateState();
            return *GLuaCxt;
        }
        return nullptr;
    }

    lua_State* GetState()
    {
        return GLuaCxt ? (lua_State*)(*GLuaCxt) : nullptr;
    }

    bool Startup()
    {
        if (GLuaCxt)
        {
            GLuaCxt->SetEnable(true);
            return true;
        }
        return false;
    }

    void Shutdown()
    {
        if (GLuaCxt)
        {
            GLuaCxt->SetEnable(false);
        }
    }

    /**
     * Lua stack index wrapper
     */
    FLuaIndex::FLuaIndex(int32 _Index)
        : Index(_Index)
    {
        if (Index < 0 && Index > LUA_REGISTRYINDEX)
        {
            int32 Top = lua_gettop(*GLuaCxt);
            Index = Top + Index + 1;
        }
    }

    /**
     * Generic Lua value
     */
    FLuaValue::FLuaValue(int32 _Index)
        : FLuaIndex(_Index)
    {
        Type = lua_type(*GLuaCxt, Index);
    }

    FLuaValue::FLuaValue(int32 _Index, int32 _Type)
        : FLuaIndex(_Index), Type(_Type)
    {
        check(lua_type(*GLuaCxt, Index) == Type);
    }

    /**
     * Lua table wrapper
     */
    FLuaTable::FLuaTable(int32 _Index)
        : FLuaIndex(_Index), PushedValues(0)
    {
        check(lua_type(*GLuaCxt, Index) == LUA_TTABLE);
    }

    FLuaTable::FLuaTable(FLuaValue Value)
        : FLuaIndex(Value.GetIndex()), PushedValues(0)
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
            lua_pop(*GLuaCxt, PushedValues);
            PushedValues = 0;
        }
    }

    int32 FLuaTable::Length() const
    {
        return lua_rawlen(*GLuaCxt, Index);
    }

    FLuaValue FLuaTable::operator[](int32 i) const
    {
        int32 Type = lua_geti(*GLuaCxt, Index, i);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](int64 i) const
    {
        int32 Type = lua_geti(*GLuaCxt, Index, i);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](double d) const
    {
        lua_pushnumber(*GLuaCxt, d);
        int32 Type = lua_gettable(*GLuaCxt, Index);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](const char *s) const
    {
        lua_pushstring(*GLuaCxt, s);
        int32 Type = lua_gettable(*GLuaCxt, Index);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](const void *p) const
    {
        lua_pushlightuserdata(*GLuaCxt, (void*)p);
        int32 Type = lua_gettable(*GLuaCxt, Index);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](FLuaIndex StackIndex) const
    {
        lua_pushvalue(*GLuaCxt, StackIndex.GetIndex());
        int32 Type = lua_gettable(*GLuaCxt, Index);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    FLuaValue FLuaTable::operator[](FLuaValue Key) const
    {
        lua_pushvalue(*GLuaCxt, Key.GetIndex());
        int32 Type = lua_gettable(*GLuaCxt, Index);
        ++PushedValues;
        return FLuaValue(-1, Type);
    }

    /**
     * Lua function wrapper
     */
    FLuaFunction::FLuaFunction(const char *GlobalFuncName)
        : FunctionRef(LUA_REFNIL)
    {
        if (!GlobalFuncName)
        {
            return;
        }

        // find global function and create a reference for the function 
        int32 Type = lua_getglobal(*GLuaCxt, GlobalFuncName);
        if (Type == LUA_TFUNCTION)
        {
            FunctionRef = luaL_ref(*GLuaCxt, LUA_REGISTRYINDEX);
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Global function %s doesn't exist!"), ANSI_TO_TCHAR(GlobalFuncName));
            lua_pop(*GLuaCxt, 1);
        }
    }

    FLuaFunction::FLuaFunction(const char *GlobalTableName, const char *FuncName)
        : FunctionRef(LUA_REFNIL)
    {
        if (!GlobalTableName || !FuncName)
        {
            return;
        }

        // find a function in a global table and create a reference for the function 
        int32 Type = lua_getglobal(*GLuaCxt, GlobalTableName);
        if (Type == LUA_TTABLE)
        {
            Type = lua_getfield(*GLuaCxt, -1, FuncName);
            if (Type == LUA_TFUNCTION)
            {
                FunctionRef = luaL_ref(*GLuaCxt, LUA_REGISTRYINDEX);
                lua_pop(*GLuaCxt, 1);
            }
            else
            {
                UE_LOG(LogUnLua, Verbose, TEXT("Function %s of global table %s doesn't exist!"), ANSI_TO_TCHAR(FuncName), ANSI_TO_TCHAR(GlobalTableName));
                lua_pop(*GLuaCxt, 2);
            }
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Global table %s doesn't exist!"), ANSI_TO_TCHAR(GlobalTableName));
            lua_pop(*GLuaCxt, 1);
        }
    }

    FLuaFunction::FLuaFunction(FLuaTable Table, const char *FuncName)
        : FunctionRef(LUA_REFNIL)
    {
        // find a function in a table and create a reference for the function 
        lua_pushstring(*GLuaCxt, FuncName);
        int32 Type = lua_gettable(*GLuaCxt, Table.GetIndex());
        if (Type == LUA_TFUNCTION)
        {
            FunctionRef = luaL_ref(*GLuaCxt, LUA_REGISTRYINDEX);
        }
        else
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Function %s doesn't exist!"), ANSI_TO_TCHAR(FuncName));
            lua_pop(*GLuaCxt, 1);
        }
    }

    FLuaFunction::FLuaFunction(FLuaValue Value)
        : FunctionRef(LUA_REFNIL)
    {
        // create a reference for the Generic Lua value if it's a function
        int32 Type = Value.GetType();
        if (Type == LUA_TFUNCTION)
        {
            lua_pushvalue(*GLuaCxt, Value.GetIndex());
            FunctionRef = luaL_ref(*GLuaCxt, LUA_REGISTRYINDEX);
        }
    }

    FLuaFunction::~FLuaFunction()
    {
        // releases reference for the function
        if (FunctionRef != LUA_REFNIL)
        {
            luaL_unref(*GLuaCxt, LUA_REGISTRYINDEX, FunctionRef);
        }
    }

    /**
     * Lua function return values
     */
    FLuaRetValues::FLuaRetValues(int32 NumResults)
        : bValid(NumResults > -1)
    {
        if (NumResults > 0)
        {
            Values.Reserve(NumResults);
            for (int32 i = 0; i < NumResults; ++i)
            {
                Values.Add(FLuaValue(i - NumResults));
            }
        }
    }

    // move constructor, and disable copy constructor
    FLuaRetValues::FLuaRetValues(FLuaRetValues &&Src)
        : Values(MoveTemp(Src.Values)), bValid(Src.bValid)
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
            lua_pop(*GLuaCxt, Values.Num());
            Values.Empty();
        }
    }

    const FLuaValue& FLuaRetValues::operator[](int32 i) const
    {
        check(i > -1 && i < Values.Num());
        return Values[i];
    }

    /**
     * Helper to check Lua stack
     */
#if !UE_BUILD_SHIPPING
    FCheckStack::FCheckStack()
        : ExpectedTopIncrease(0)
    {
        OldTop = lua_gettop(*GLuaCxt);
    }

    FCheckStack::FCheckStack(int32 _ExpectedTopIncrease)
        : ExpectedTopIncrease(_ExpectedTopIncrease)
    {
        OldTop = lua_gettop(*GLuaCxt);
    }

    FCheckStack::~FCheckStack()
    {
        int32 Top = lua_gettop(*GLuaCxt);
        if (Top != OldTop + ExpectedTopIncrease)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Lua Top Increase : %d"), Top - OldTop - ExpectedTopIncrease);
        }
    }
#endif

    /**
     * Helper to recover Lua stack automatically
     */
    FAutoStack::FAutoStack()
    {
        OldTop = lua_gettop(*GLuaCxt);
    }

    FAutoStack::~FAutoStack()
    {
        lua_settop(*GLuaCxt, OldTop);
    }

    /**
     * Exported enum
     */
    void FExportedEnum::Register(lua_State *L)
    {
        TStringConversion<TStringConvert<TCHAR, ANSICHAR>> EnumName(*Name);

        int32 Type = luaL_getmetatable(L, EnumName.Get());
        lua_pop(L, 1);
        if (Type == LUA_TTABLE)
        {
            return;
        }

        luaL_newmetatable(L, EnumName.Get());

        for (TMap<FString, int32>::TIterator It(NameValues); It; ++It)
        {
            lua_pushstring(L, TCHAR_TO_ANSI(*It.Key()));
            lua_pushinteger(L, It.Value());
            lua_rawset(L, -3);
        }

#if WITH_UE4_NAMESPACE
        lua_getglobal(L, "UE4");
        lua_pushstring(L, EnumName.Get());
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);
        lua_pop(L, 2);
#else
        lua_setglobal(L, EnumName.Get());
#endif
    }

#if WITH_EDITOR
    void FExportedEnum::GenerateIntelliSense(FString &Buffer) const
    {
        Buffer = FString::Printf(TEXT("---@class %s\r\n"), *Name);

        for (TMap<FString, int32>::TConstIterator It(NameValues); It; ++It)
        {
            Buffer += FString::Printf(TEXT("---@field %s %s %s\r\n"), TEXT("public"), *It.Key(), TEXT("integer"));
        }

        Buffer += FString::Printf(TEXT("local %s = {}\r\n\r\n"), *Name);
        Buffer += FString::Printf(TEXT("return %s\r\n"), *Name);
    }
#endif

} // namespace UnLua

/**
 * Try to 'hotfix' Lua.
 * 1. Try to execute developers registered callback
 * 2. Try to call global Lua function 'HotFix'
 */
bool HotfixLua()
{
    if (GLuaCxt)
    {
        if (FUnLuaDelegates::HotfixLua.IsBound())
        {
            FUnLuaDelegates::HotfixLua.Execute(*GLuaCxt);
            return true;
        }

        UnLua::FLuaRetValues RetValues = UnLua::Call(*GLuaCxt, "HotFix");
        return RetValues.IsValid();
    }
    return false;
}

FString GLuaSrcRelativePath = TEXT("Script/");
FString GLuaSrcFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + GLuaSrcRelativePath);
