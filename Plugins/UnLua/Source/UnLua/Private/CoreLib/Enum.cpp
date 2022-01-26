#include "Enum.h"
#include "lua.hpp"
#include "LuaCore.h"
#include "UnLua.h"
#include "ReflectionUtils/ReflectionRegistry.h"

/**
 * __index meta methods for enum
 */
int32 Enum_Index(lua_State* L)
{
    // 1: meta table of the Enum; 2: entry name in Enum

    check(lua_isstring(L, -1));
    lua_pushstring(L, "__name"); // 3
    lua_rawget(L, 1); // 3
    check(lua_isstring(L, -1));

    const FEnumDesc* Enum = GReflectionRegistry.FindEnum(lua_tostring(L, -1));
    if ((!Enum)
        || (!Enum->IsValid()))
    {
        lua_pop(L, 1);
        return 0;
    }
    int64 Value = Enum->GetValue(lua_tostring(L, 2));

    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    lua_pushinteger(L, Value);
    lua_rawset(L, 1);
    lua_pushinteger(L, Value);

    return 1;
}

int32 Enum_Delete(lua_State* L)
{
    lua_pushstring(L, "__name");
    int32 Type = lua_rawget(L, 1);
    if (Type == LUA_TSTRING)
    {
        const char* EnumName = lua_tostring(L, -1);
        const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
        if (EnumDesc)
        {
            GReflectionRegistry.UnRegisterEnum(EnumDesc);
        }
    }
    lua_pop(L, 1);
    return 0;
}

int32 Enum_GetMaxValue(lua_State* L)
{
    int32 MaxValue = 0;

    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L, -1) == LUA_TTABLE)
    {
        lua_pushstring(L, "__name");
        int32 Type = lua_rawget(L, -2);
        if (Type == LUA_TSTRING)
        {
            const char* EnumName = lua_tostring(L, -1);
            const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
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

int32 Enum_GetNameByValue(lua_State* L)
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
            const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
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

void UnLua::CoreLib::RegisterEnum(lua_State* L, FEnumDesc* EnumDesc)
{
    TStringConversion<TStringConvert<TCHAR, ANSICHAR>> EnumName(*EnumDesc->GetName());
    int32 Type = luaL_getmetatable(L, EnumName.Get());
    if (Type != LUA_TTABLE)
    {
        luaL_newmetatable(L, EnumName.Get());

        lua_pushstring(L, "__index");
        lua_pushcfunction(L, Enum_Index);
        lua_rawset(L, -3);

        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, Enum_Delete);
        lua_rawset(L, -3);

        // add other members here
        lua_pushstring(L, "GetMaxValue");
        lua_pushvalue(L, -2); // EnumTable
        lua_pushcclosure(L, Enum_GetMaxValue, 1); // closure
        lua_rawset(L, -3);

        lua_pushstring(L, "GetNameByValue");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, Enum_GetNameByValue, 1);
        lua_rawset(L, -3);

        lua_pushvalue(L, -1); // set metatable to self
        lua_setmetatable(L, -2);

        SetTableForClass(L, EnumName.Get());
    }
    lua_pop(L, 1);
}
