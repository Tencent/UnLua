#include <lua.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>

#include "Userdata.hpp"
#include "values.hpp"

#pragma push_macro("check")
#undef check

using namespace rapidjson;

template<>
const char* const Userdata<SchemaDocument>::metatable()
{
	return "rapidjson.SchemaDocument";
}

template<>
SchemaDocument* Userdata<SchemaDocument>::construct(lua_State * L)
{
	switch (lua_type(L, 1)) {
	case LUA_TNONE:
		return new SchemaDocument(Document()); // empty schema
	case LUA_TSTRING: {
		Document doc;
		size_t len = 0;
		const char* s = lua_tolstring(L, 1, &len);
		doc.Parse(s, len);
		return new SchemaDocument(doc);
	}
	case LUA_TTABLE: {
		Document doc;
		values::toDocument(L, 1, &doc);
		return new SchemaDocument(doc);
	}
	case LUA_TUSERDATA: {
		Document* doc = Userdata<Document>::check(L, 1);
		return new SchemaDocument(*doc);
	}
	default:
		luax::typerror(L, 1, "none, string, table or rapidjson.Document");
		return NULL; // Just make compiler happy
	}
}



template <>
const luaL_Reg* Userdata<SchemaDocument>::methods() {
	static const luaL_Reg reg[] = {
		{ "__gc", metamethod_gc },

		{ NULL, NULL }
	};
	return reg;
}


template<>
const char* const Userdata<SchemaValidator>::metatable()
{
	return "rapidjson.SchemaValidator";
}

template<>
SchemaValidator* Userdata<SchemaValidator>::construct(lua_State * L)
{
	SchemaDocument* sd = Userdata<SchemaDocument>::check(L, 1);
	return new SchemaValidator(*sd);
}


static void pushValidator_error(lua_State* L, SchemaValidator* validator) {
	luaL_Buffer b;
	luaL_buffinit(L, &b);

	luaL_addstring(&b, "invalid \"");

	luaL_addstring(&b, validator->GetInvalidSchemaKeyword());
	luaL_addstring(&b, "\" in document at pointer \"");

	// document pointer
	StringBuffer sb;
	validator->GetInvalidDocumentPointer().StringifyUriFragment(sb);
	luaL_addlstring(&b, sb.GetString(), sb.GetSize());
	luaL_addchar(&b, '"');

	luaL_pushresult(&b);
}


static int SchemaValidator_validate(lua_State* L) {
	SchemaValidator* validator = Userdata<SchemaValidator>::check(L, 1);
	Document* doc = Userdata<Document>::check(L, 2);
	bool ok = doc->Accept(*validator);
	lua_pushboolean(L, ok);
	int nr;
	if (ok)
		nr = 1;
	else
	{
		pushValidator_error(L, validator);
		nr = 2;
	}
	validator->Reset();

	return nr;
}


template <>
const luaL_Reg* Userdata<SchemaValidator>::methods() {
	static const luaL_Reg reg[] = {
		{ "__gc", metamethod_gc },
		{ "validate", SchemaValidator_validate },
		{ NULL, NULL }
	};
	return reg;
}

#pragma pop_macro("check")