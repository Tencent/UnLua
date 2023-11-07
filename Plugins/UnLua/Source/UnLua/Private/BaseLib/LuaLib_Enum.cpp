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

#include "BaseLib/LuaLib_Enum.h"
#include "CollisionHelper.h"
#include "LuaEnv.h"
#include "UnLuaLegacy.h"
#include "UnLuaPrivate.h"

/**
 * __index meta methods for enum
 */
int32 Enum_Index(lua_State *L)
{
    const auto NumParams = lua_gettop(L);
    if (NumParams < 2)
        return 0;

    // 1: meta table of the Enum; 2: entry name in Enum
    if (lua_type(L, 1) != LUA_TTABLE)
        return 0;

    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    lua_pushstring(L, "__name");
    lua_rawget(L, 1);
    check(lua_isstring(L, -1));

    const auto Enum = UnLua::FLuaEnv::FindEnv(L)->GetEnumRegistry()->Find(lua_tostring(L, -1));
    if (!Enum)
    {
        lua_pop(L, 1);
        return 0;
    }

    Enum->Load();

    const auto Value = Enum->GetValue(lua_tostring(L, 2));

    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    lua_pushinteger(L, Value);
    lua_rawset(L, 1);
    lua_pushinteger(L, Value);

    return 1;
}

int32 Enum_GetMaxValue(lua_State* L)
{   
    int32 MaxValue = 0;
    
    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L,-1) == LUA_TTABLE)
    {
		lua_pushstring(L, "__name");
		int32 Type = lua_rawget(L, -2);
		if (Type == LUA_TSTRING)
		{
			const char* EnumName = lua_tostring(L, -1);
            const auto EnumDesc = UnLua::FLuaEnv::FindEnv(L)->GetEnumRegistry()->Find(EnumName);
			if (EnumDesc)
			{
				UEnum* Enum = EnumDesc->GetEnum();
				if (Enum)
				{
					MaxValue = Enum->GetMaxEnumValue();
				}
			}
		}
		lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_pushinteger(L, MaxValue);
    return 1;
}

int32 Enum_GetNameStringByValue(lua_State* L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }

    FString ValueName;

    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L, -1) == LUA_TTABLE)
    {
        // enum value
        int64 Value = lua_tointegerx(L, -2, nullptr);

        lua_pushstring(L, "__name");
        int32 Type = lua_rawget(L, -2);
        if (Type == LUA_TSTRING)
        {
            const char* EnumName = lua_tostring(L, -1);
            const auto EnumDesc = UnLua::FLuaEnv::FindEnv(L)->GetEnumRegistry()->Find(EnumName);
            if (EnumDesc)
            {
                UEnum* Enum = EnumDesc->GetEnum();
                if (Enum)
                {   
                    ValueName = Enum->GetNameStringByValue(Value);
                }
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    UnLua::Push(L, ValueName);

    return 1;
}

int32 Enum_GetDisplayNameTextByValue(lua_State* L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }

    FText ValueName;

    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L, -1) == LUA_TTABLE)
    {
        // enum value
        int64 Value = lua_tointegerx(L, -2, nullptr);

        lua_pushstring(L, "__name");
        int32 Type = lua_rawget(L, -2);
        if (Type == LUA_TSTRING)
        {
            const char* EnumName = lua_tostring(L, -1);
            const auto EnumDesc = UnLua::FLuaEnv::FindEnv(L)->GetEnumRegistry()->Find(EnumName);
            if (EnumDesc)
            {
                UEnum* Enum = EnumDesc->GetEnum();
                if (Enum)
                {   
                    ValueName = Enum->GetDisplayNameTextByValue(Value);
                }
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    UnLua::Push(L, ValueName);

    return 1;
}

/**
 * __index meta methods for collision related enums
 */
int32 CollisionEnum_Index(lua_State* L, int32 (*Converter)(FName))
{
    const char* Name = lua_tostring(L, -1);
    if (Name)
    {
        int32 Value = Converter(FName(Name));
        if (Value == INDEX_NONE)
        {
            UNLUA_LOGWARNING(L, LogUnLua, Warning, TEXT("%s: Cann't find enum %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Name));
        }
        lua_pushvalue(L, 2);
        lua_pushinteger(L, Value);
        lua_rawset(L, 1);
        lua_pushinteger(L, Value);
    }
    else
    {
        lua_pushinteger(L, INDEX_NONE);
    }
    return 1;
}

int32 ECollisionChannel_Index(lua_State* L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToCollisionChannel);
}

int32 EObjectTypeQuery_Index(lua_State* L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToObjectType);
}

int32 ETraceTypeQuery_Index(lua_State* L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToTraceType);
}
