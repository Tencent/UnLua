#ifndef __LUA_RAPIDJSION_LUACOMPAT_H__
#define __LUA_RAPIDJSION_LUACOMPAT_H__

#include <cmath>
#include <lua.hpp>

namespace luax {
	inline void setfuncs(lua_State* L, const luaL_Reg* funcs) {
#if LUA_VERSION_NUM >= 502 // LUA 5.2 or above
		luaL_setfuncs(L, funcs, 0);
#else
		luaL_register(L, NULL, funcs);
#endif
	}

	inline size_t rawlen(lua_State* L, int idx) {
#if LUA_VERSION_NUM >= 502
		return lua_rawlen(L, idx);
#else
		return lua_objlen(L, idx);
#endif
	}

	inline int absindex(lua_State *L, int idx) {
#if LUA_VERSION_NUM >= 502
        return lua_absindex(L, idx);
#else
        if (idx < 0 && idx > LUA_REGISTRYINDEX)
            idx += lua_gettop(L) + 1;
        return idx;
#endif

    }

	inline bool isinteger(lua_State* L, int idx, int64_t* out = NULL)
	{
#if LUA_VERSION_NUM >= 503
		if (lua_isinteger(L, idx)) // but it maybe not detect all integers.
		{
			if (out)
				*out = lua_tointeger(L, idx);
			return true;
		}
#endif
		double intpart;
		if (std::modf(lua_tonumber(L, idx), &intpart) == 0.0)
		{
			if (std::numeric_limits<lua_Integer>::min() <= intpart
				&& intpart <= (double)std::numeric_limits<lua_Integer>::max())
			{
				if (out)
					*out = static_cast<int64_t>(intpart);
				return true;
			}
		}
		return false;
	}

	inline int typerror(lua_State* L, int narg, const char* tname) {
#if LUA_VERSION_NUM < 502
		return luaL_typerror(L, narg, tname);
#else
		const char *msg = lua_pushfstring(L, "%s expected, got %s",
			tname, luaL_typename(L, narg));
		return luaL_argerror(L, narg, msg);
#endif
	}

	inline bool optboolfield(lua_State* L, int idx, const char* name, bool def)
	{
		bool v = def;
		int t = lua_type(L, idx);
		if (t != LUA_TTABLE && t != LUA_TNONE)
			luax::typerror(L, idx, "table");

		if (t != LUA_TNONE) {
			lua_getfield(L, idx, name);  // [field]
			if (!lua_isnoneornil(L, -1))
				v = lua_toboolean(L, -1) != 0;;
			lua_pop(L, 1);
		}

		return v;
	}

	inline int optintfield(lua_State* L, int idx, const char* name, int def)
	{
		int v = def;
		lua_getfield(L, idx, name);  // [field]
		if (lua_isnumber(L, -1))
			v = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);
		return v;
	}

}





#endif // __LUA_RAPIDJSION_LUACOMPAT_H__
