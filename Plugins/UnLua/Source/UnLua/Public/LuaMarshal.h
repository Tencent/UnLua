#pragma once

#include "UnLuaEx.h"
#include "CoreMinimal.h"

namespace UnLua
{
    /*template <typename T> bool PullFromLua(lua_State *L, int Index, T &Data)
    {
        return true;
    }*/

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, bool &Data)
    {
        if (lua_isboolean(L, Index))
        {
            Data = lua_toboolean(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, unsigned char &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, char &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, short &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, unsigned short &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, int32 &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, uint32 &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, int64 &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, uint64 &Data)
    {
        if (lua_isinteger(L, Index))
        {
            Data = lua_tointeger(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, float &Data)
    {
        if (lua_isnumber(L, Index))
        {
            Data = lua_tonumber(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, double &Data)
    {
        if (lua_isnumber(L, Index))
        {
            Data = lua_tonumber(L, Index);
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, FString &Data)
    {
        if (lua_isstring(L, Index))
        {
            Data = UTF8_TO_TCHAR(lua_tostring(L, Index));
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, FName &Data)
    {
        if (lua_isstring(L, Index))
        {
            Data = FName(lua_tostring(L, Index));
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, FText &Data)
    {
        if (lua_isstring(L, Index))
        {
            Data = FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, Index)));
            return true;
        }
        return false;
    }

    FORCEINLINE bool PullFromLua(lua_State *L, int Index, UObject *&Data)
    {
        if (lua_isuserdata(L, Index))
        {
            Data = UnLua::GetUObject(L, Index);
            return true;
        }
        return false;
    }

    template <typename T> bool PullFromLua(lua_State *L, int Index, T *&Data)
    {
        if (lua_isuserdata(L, Index))
        {
            Data = (T*)GetPointer(L, Index); //TODO: add type check
            return true;
        }
        return false;
    }

    /*template <typename T> void PushToLua(lua_State *L, T &Data)
    {
    }*/

    FORCEINLINE void PushToLua(lua_State *L, bool Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, unsigned char Data)
    {
        Push(L, (int)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, char Data)
    {
        Push(L, (int)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, short Data)
    {
        Push(L, (int)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, unsigned short Data)
    {
        Push(L, (int)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, int32 Data)
    {
        Push(L, (int)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, uint32 Data)
    {
        Push(L, (int64)Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, int64 Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, uint64 Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, float Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, double Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, FString& Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, FName& Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, FText& Data)
    {
        Push(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, UObject *Data)
    {
        UnLua::PushUObject(L, Data);
    }

    FORCEINLINE void PushToLua(lua_State *L, void *Data)
    {
        lua_pushlightuserdata(L, Data);
    }

    template <typename T> void PushToLuaByValue(lua_State *L, T &Data)
    {
        void *Userdata = NewUserdata(L, sizeof(T), UnLua::TType<T>::GetName(), alignof(T));
        new(Userdata) T(Data);
    }
}
