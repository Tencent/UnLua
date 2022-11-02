#ifndef LUASOCKET_H
#define LUASOCKET_H
/*=========================================================================*\
* LuaSocket toolkit
* Networking support for the Lua language
* Diego Nehab
* 9/11/1999
\*=========================================================================*/

/*-------------------------------------------------------------------------* \
* Current socket library version
\*-------------------------------------------------------------------------*/
#define LUASOCKET_VERSION    "LuaSocket 3.0-rc1"
#define LUASOCKET_COPYRIGHT  "Copyright (C) 1999-2013 Diego Nehab"

/*-------------------------------------------------------------------------*\
* This macro prefixes all exported API functions
\*-------------------------------------------------------------------------*/
#undef LUASOCKET_API
#ifdef _WIN32
#define LUASOCKET_API __declspec(dllexport)
#else
#define LUASOCKET_API __attribute__ ((visibility ("default")))
#endif

#include "lua.hpp"

/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
LUASOCKET_API int luaopen_socket_core(lua_State *L);

#endif /* LUASOCKET_H */
