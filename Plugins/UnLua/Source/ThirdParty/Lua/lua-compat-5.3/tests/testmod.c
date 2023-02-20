#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "compat-5.3.h"


static int test_isinteger (lua_State *L) {
  lua_pushboolean(L, lua_isinteger(L, 1));
  return 1;
}


static int test_rotate (lua_State *L) {
  int r = (int)luaL_checkinteger(L, 1);
  int n = lua_gettop(L)-1;
  luaL_argcheck(L, (r < 0 ? -r : r) <= n, 1, "not enough arguments");
  lua_rotate(L, 2, r);
  return n;
}


static int test_str2num (lua_State *L) {
  const char *s = luaL_checkstring(L, 1);
  size_t len = lua_stringtonumber(L, s);
  if (len == 0)
    lua_pushnumber(L, 0);
  lua_pushinteger(L, (lua_Integer)len);
  return 2;
}


static int my_mod (lua_State *L ) {
  lua_newtable(L);
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "boolean");
  return 1;
}

static int test_requiref (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_newtable(L);
  lua_pushboolean(L, 0);
  lua_setfield(L, -2, "boolean");
  lua_setfield(L, -2, "requiref3");
  lua_pop(L, 1);
  luaL_requiref(L, "requiref1", my_mod, 0);
  luaL_requiref(L, "requiref2", my_mod, 1);
  luaL_requiref(L, "requiref3", my_mod, 1);
  return 3;
}

static int test_getseti (lua_State *L) {
  lua_Integer k = luaL_checkinteger(L, 2);
  lua_Integer n = 0;
  if (lua_geti(L, 1, k) == LUA_TNUMBER) {
    n = lua_tointeger(L, -1);
  } else {
    lua_pop(L, 1);
    lua_pushinteger(L, n);
  }
  lua_pushinteger(L, n+1);
  lua_seti(L, 1, k);
  return 1;
}


#ifndef LUA_EXTRASPACE
#define LUA_EXTRASPACE (sizeof(void*))
#endif

static int test_getextraspace (lua_State *L) {
  size_t len = 0;
  char const* s = luaL_optlstring(L, 1, NULL, &len);
  char* p = (char*)lua_getextraspace(L);
  lua_pushstring(L, p);
  if (s)
    memcpy(p, s, len > LUA_EXTRASPACE-1 ? LUA_EXTRASPACE-1 : len+1);
  return 1;
}


/* additional tests for Lua5.1 */
#define NUP 3

static int test_newproxy (lua_State *L) {
  lua_settop(L, 0);
  lua_newuserdata(L, 0);
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -3);
  return 2;
}

static int test_absindex (lua_State *L) {
  int i = 1;
  for (i = 1; i <= NUP; ++i)
    lua_pushvalue(L, lua_absindex(L, lua_upvalueindex(i)));
  lua_pushvalue(L, lua_absindex(L, LUA_REGISTRYINDEX));
  lua_pushstring(L, lua_typename(L, lua_type(L, lua_absindex(L, -1))));
  lua_replace(L, lua_absindex(L, -2));
  lua_pushvalue(L, lua_absindex(L, -2));
  lua_pushvalue(L, lua_absindex(L, -4));
  lua_pushvalue(L, lua_absindex(L, -6));
  i += 3;
  lua_pushvalue(L, lua_absindex(L, 1));
  lua_pushvalue(L, lua_absindex(L, 2));
  lua_pushvalue(L, lua_absindex(L, 3));
  i += 3;
  return i;
}

static int test_arith (lua_State *L) {
  lua_settop(L, 2);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPADD);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPSUB);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPMUL);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPDIV);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPMOD);
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_arith(L, LUA_OPPOW);
  lua_pushvalue(L, 1);
  lua_arith(L, LUA_OPUNM);
  return lua_gettop(L)-2;
}

static int test_compare (lua_State *L) {
  luaL_checknumber(L, 1);
  luaL_checknumber(L, 2);
  lua_settop(L, 2);
  lua_pushboolean(L, lua_compare(L, 1, 2, LUA_OPEQ));
  lua_pushboolean(L, lua_compare(L, 1, 2, LUA_OPLT));
  lua_pushboolean(L, lua_compare(L, 1, 2, LUA_OPLE));
  return 3;
}

static int test_globals (lua_State *L) {
  lua_pushglobaltable(L);
  return 1;
}

static int test_tonumber (lua_State *L) {
  int isnum = 0;
  lua_Number n = lua_tonumberx(L, 1, &isnum);
  if (!isnum)
    lua_pushnil(L);
  else
    lua_pushnumber(L, n);
  return 1;
}

static int test_tointeger (lua_State *L) {
  int isnum = 0;
  lua_Integer n = lua_tointegerx(L, 1, &isnum);
  if (!isnum)
    lua_pushnil(L);
  else
    lua_pushinteger(L, n);
  lua_pushinteger(L, lua_tointeger(L, 1));
  return 2;
}

static int test_len (lua_State *L) {
  luaL_checkany(L, 1);
  lua_len(L, 1);
  lua_pushinteger(L, luaL_len(L, 1));
  return 2;
}

static int test_copy (lua_State *L) {
  int args = lua_gettop(L);
  if (args >= 2) {
    int i = 0;
    for (i = args-1; i > 0; --i)
      lua_copy(L, args, i);
  }
  return args;
}

/* need an address */
static char const dummy = 0;

static int test_rawxetp (lua_State *L) {
  if (lua_gettop(L) > 0)
    lua_pushvalue(L, 1);
  else
    lua_pushliteral(L, "hello again");
  lua_rawsetp(L, LUA_REGISTRYINDEX, &dummy);
  lua_settop(L, 0);
  lua_rawgetp(L, LUA_REGISTRYINDEX, &dummy);
  return 1;
}

static int test_udata (lua_State *L) {
  const char *tname = luaL_optstring(L, 1, "utype1");
  void *u1 = lua_newuserdata(L, 1);
  int u1pos = lua_gettop(L);
  void *u2 = lua_newuserdata(L, 1);
  int u2pos = lua_gettop(L);
  luaL_newmetatable(L, "utype1");
  luaL_newmetatable(L, "utype2");
  lua_pop(L, 2);
  luaL_setmetatable(L, "utype2");
  lua_pushvalue(L, u1pos);
  luaL_setmetatable(L, "utype1");
  lua_pop(L, 1);
  (void)u1;
  (void)u2;
  lua_pushlightuserdata(L, luaL_testudata(L, u1pos, tname));
  lua_pushlightuserdata(L, luaL_testudata(L, u2pos, tname));
  luaL_getmetatable(L, "utype1");
  lua_getfield(L, -1, "__name");
  lua_replace(L, -2);
  return 3;
}

static int test_subtable (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 1);
  if (luaL_getsubtable(L, 1, "xxx")) {
    lua_pushliteral(L, "oldtable");
  } else {
    lua_pushliteral(L, "newtable");
  }
  return 2;
}

static int test_uservalue (lua_State *L) {
  void *udata = lua_newuserdata(L, 1);
  int ui = lua_gettop(L);
  lua_newtable(L);
  lua_setuservalue(L, ui);
  lua_pushinteger(L, lua_getuservalue(L, ui));
  (void)udata;
  return 2;
}

static int test_upvalues (lua_State *L) {
  int i = 1;
  for (i = 1; i <= NUP; ++i)
    lua_pushvalue(L, lua_upvalueindex(i));
  return NUP;
}

static int test_tolstring (lua_State *L) {
  size_t len = 0;
  luaL_tolstring(L, 1, &len);
  lua_pushinteger(L, (int)len);
  return 2;
}

static int test_pushstring (lua_State *L) {
  lua_pushstring(L, lua_pushliteral(L, "abc"));
  lua_pushstring(L, lua_pushlstring(L, "abc", 2));
  lua_pushstring(L, lua_pushlstring(L, NULL, 0));
  lua_pushstring(L, lua_pushstring(L, "abc"));
  lua_pushboolean(L, NULL == lua_pushstring(L, NULL));
  return 10;
}

static int test_buffer (lua_State *L) {
  luaL_Buffer b;
  char *p = luaL_buffinitsize(L, &b, LUAL_BUFFERSIZE+1);
  p[0] = 'a';
  p[1] = 'b';
  luaL_addsize(&b, 2);
  luaL_addstring(&b, "c");
  lua_pushliteral(L, "d");
  luaL_addvalue(&b);
  luaL_addchar(&b, 'e');
  luaL_pushresult(&b);
  return 1;
}

static int test_exec (lua_State *L) {
  const char *cmd = luaL_checkstring(L, 1);
  errno = 0;
  return luaL_execresult(L, system(cmd));
}

static int test_loadstring (lua_State *L) {
  size_t len = 0;
  char const* s = luaL_checklstring(L, 1, &len);
  char const* mode = luaL_optstring(L, 2, "bt");
  lua_pushinteger(L, luaL_loadbufferx(L, s, len, s, mode));
  return 2;
}

static int test_loadfile (lua_State *L) {
  char filename[L_tmpnam+1] = { 0 };
  size_t len = 0;
  char const* s = luaL_checklstring(L, 1, &len);
  char const* mode = luaL_optstring(L, 2, "bt");
  if (tmpnam(filename)) {
    FILE* f = fopen(filename, "wb");
    if (f) {
      fwrite(s, 1, len, f);
      fclose(f);
      lua_pushinteger(L, luaL_loadfilex(L, filename, mode));
      remove(filename);
      return 2;
    } else
      remove(filename);
  }
  return 0;
}


static const luaL_Reg funcs[] = {
  { "isinteger", test_isinteger },
  { "rotate", test_rotate },
  { "strtonum", test_str2num },
  { "requiref", test_requiref },
  { "getseti", test_getseti },
  { "extraspace", test_getextraspace },
  { "newproxy", test_newproxy },
  { "arith", test_arith },
  { "compare", test_compare },
  { "tonumber", test_tonumber },
  { "tointeger", test_tointeger },
  { "len", test_len },
  { "copy", test_copy },
  { "rawxetp", test_rawxetp },
  { "subtable", test_subtable },
  { "udata", test_udata },
  { "uservalue", test_uservalue },
  { "globals", test_globals },
  { "tolstring", test_tolstring },
  { "pushstring", test_pushstring },
  { "buffer", test_buffer },
  { "exec", test_exec },
  { "loadstring", test_loadstring },
  { "loadfile", test_loadfile },
  { NULL, NULL }
};

static const luaL_Reg more_funcs[] = {
  { "getupvalues", test_upvalues },
  { "absindex", test_absindex },
  { NULL, NULL }
};


#ifdef __cplusplus
extern "C" {
#endif
int luaopen_testmod (lua_State *L) {
  int i = 1;
  luaL_newlib(L, funcs);
  for (i = 1; i <= NUP; ++i)
    lua_pushnumber(L, i);
  luaL_setfuncs(L, more_funcs, NUP);
  return 1;
}
#ifdef __cplusplus
}
#endif

