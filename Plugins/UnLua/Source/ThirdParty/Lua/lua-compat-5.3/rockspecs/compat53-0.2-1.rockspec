package = "compat53"
version = "0.2-1"
source = {
   url = "https://github.com/lunarmodules/lua-compat-5.3/archive/v0.2.zip",
   dir = "lua-compat-5.3-0.2",
}
description = {
   summary = "Compatibility module providing Lua-5.3-style APIs for Lua 5.2 and 5.1",
   detailed = [[
      This is a small module that aims to make it easier to write Lua
      code in a Lua-5.3-style that runs on Lua 5.3, 5.2, and 5.1.
      It does *not* make Lua 5.2 (or even 5.1) entirely compatible
      with Lua 5.3, but it brings the API closer to that of Lua 5.3.
   ]],
   homepage = "https://github.com/lunarmodules/lua-compat-5.3",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1, < 5.4",
   --"struct" -- make Roberto's struct module optional
}
build = {
   type = "builtin",
   modules = {
      ["compat53.init"] = "compat53/init.lua",
      ["compat53.module"] = "compat53/module.lua",
      ["compat53.utf8"] = "lutf8lib.c",
      ["compat53.table"] = "ltablib.c",
      ["compat53.string"] = "lstrlib.c",
   }
}

