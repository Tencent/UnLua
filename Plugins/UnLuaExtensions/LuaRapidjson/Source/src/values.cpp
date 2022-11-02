#include "values.hpp"
#include "luax.hpp"


using rapidjson::Value;
using rapidjson::SizeType;


namespace values {

	namespace details {
		static Value NumberValue(lua_State* L, int idx);
		static Value StringValue(lua_State* L, int idx, Allocator& allocator);
		static Value TableValue(lua_State* L, int idx, int depth, Allocator& allocator);
		static Value ObjectValue(lua_State* L, int idx, int depth, Allocator& allocator);
		static Value ArrayValue(lua_State* L, int idx, int depth, Allocator& allocator);


		Value toValue(lua_State* L, int idx, int depth, Allocator& allocator) {
			int t = lua_type(L, idx);
			switch (t) {
			case LUA_TBOOLEAN:
				return Value(lua_toboolean(L, idx) != 0);
			case LUA_TNUMBER:
				return NumberValue(L, idx);
			case LUA_TSTRING:
				return StringValue(L, idx, allocator);
			case LUA_TTABLE:
				return TableValue(L, idx, depth + 1, allocator);
			case LUA_TNIL:
				return Value();
			case LUA_TLIGHTUSERDATA:
                if (isnull(L, idx))
                    return Value();
            // otherwise fall thought
			case LUA_TFUNCTION: // fall thought
			case LUA_TUSERDATA: // fall thought
			case LUA_TTHREAD: // fall thought
			case LUA_TNONE: // fall thought
			default:
				luaL_error(L, "value type %s is not a valid json value", lua_typename(L, t));
				return Value(); // Just make compiler happy
			}
		}

		Value NumberValue(lua_State* L, int idx) {
			int64_t integer;
			return luax::isinteger(L, idx, &integer) ? Value(integer) : Value(lua_tonumber(L, idx));
		}

		Value StringValue(lua_State* L, int idx, Allocator& allocator) {
			size_t len;
			const char* s = lua_tolstring(L, idx, &len);
			return Value(s, static_cast<SizeType>(len), allocator);
		}

		Value TableValue(lua_State* L, int idx, int depth, Allocator& allocator)
		{
			if (depth > 1024)
				luaL_error(L, "nested too depth");

			if (!lua_checkstack(L, 4)) // requires at least 4 slots in stack: table, key, value, key
				luaL_error(L, "stack overflow");

			return isarray(L, idx) ? ArrayValue(L, idx, depth, allocator) : ObjectValue(L, idx, depth, allocator);
		}

		Value ObjectValue(lua_State* L, int idx, int depth, Allocator& allocator)
		{
			Value object(rapidjson::kObjectType);
			idx = luax::absindex(L, idx);
			lua_pushnil(L);						// [nil]
			while (lua_next(L, idx))
			{
				// [key, value]
				if (lua_type(L, -2) == LUA_TSTRING)
				{
					object.AddMember(StringValue(L, -2, allocator), toValue(L, -1, depth, allocator), allocator);
				}

				// pop value, leaving original key
				lua_pop(L, 1);
				// [key]
			}
			// []

			return object;
		}

		Value ArrayValue(lua_State* L, int idx, int depth, Allocator& allocator)
		{
			Value array(rapidjson::kArrayType);
			int MAX = static_cast<int>(luax::rawlen(L, idx)); // luax::rawlen always returns size_t (>= 0)
			for (int n = 1; n <= MAX; ++n)
			{
				lua_rawgeti(L, idx, n); // [table, element]
				array.PushBack(toValue(L, -1, depth, allocator), allocator);
				lua_pop(L, 1); // [table]
			}
			// [table]

			return array;
		}
	}

}
