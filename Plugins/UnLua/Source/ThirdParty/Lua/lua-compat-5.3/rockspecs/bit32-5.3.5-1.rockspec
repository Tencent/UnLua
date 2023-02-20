package = "bit32"
version = "5.3.5-1"
source = {
   url = "https://github.com/lunarmodules/lua-compat-5.3/archive/v0.9.zip",
   dir = "lua-compat-5.3-0.9",
}
description = {
   summary = "Lua 5.2 bit manipulation library",
   detailed = [[
      bit32 is the native Lua 5.2 bit manipulation library, in the version
      from Lua 5.3; it is compatible with Lua 5.1, 5.2 and 5.3.
   ]],
   homepage = "http://www.lua.org/manual/5.2/manual.html#6.7",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1, < 5.5"
}
build = {
   type = "builtin",
   modules = {
      bit32 = {
         sources = { "lbitlib.c" },
         defines = { "LUA_COMPAT_BITLIB" },
         incdirs = { "c-api" },
      }
   }
}
