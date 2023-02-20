[![Compat53 Status](https://github.com/lunarmodules/lua-compat-5.3/workflows/compat53-tests/badge.svg)](https://github.com/lunarmodules/lua-compat-5.3/actions?workflow=compat53-tests)
[![Bit32 Status](https://github.com/lunarmodules/lua-compat-5.3/workflows/bit32-multi-arch-tests/badge.svg)](https://github.com/lunarmodules/lua-compat-5.3/actions?workflow=bit32-multi-arch-tests)

# lua-compat-5.3

Lua-5.3-style APIs for Lua 5.2 and 5.1.

## What is it

This is a small module that aims to make it easier to write code
in a Lua-5.3-style that is compatible with Lua 5.1, Lua 5.2, and Lua
5.3. This does *not* make Lua 5.2 (or even Lua 5.1) entirely
compatible with Lua 5.3, but it brings the API closer to that of Lua
5.3.

It includes:

* _For writing Lua_: The Lua module `compat53`, which can be require'd
  from Lua scripts and run in Lua 5.1, 5.2, and 5.3, including a
  backport of the `utf8` module, the 5.3 `table` module, and the
  string packing functions straight from the Lua 5.3 sources.
* _For writing C_: A C header and file which can be linked to your
  Lua module written in C, providing some functions from the C API
  of Lua 5.3 that do not exist in Lua 5.2 or 5.1, making it easier to
  write C code that compiles with all three versions of liblua.

## How to use it

### Lua module

```lua
require("compat53")
```

`compat53` makes changes to your global environment and does not return
a meaningful return value, so the usual idiom of storing the return of
`require` in a local variable makes no sense.

When run under Lua 5.3+, this module does nothing.

When run under Lua 5.2 or 5.1, it replaces some of your standard
functions and adds new ones to bring your environment closer to that
of Lua 5.3. It also tries to load the backported `utf8`, `table`, and
string packing modules automatically. If unsuccessful, pure Lua
versions of the new `table` functions are used as a fallback, and
[Roberto's struct library][1] is tried for string packing.

#### Lua submodules

```lua
local _ENV = require("compat53.module")
if setfenv then setfenv(1, _ENV) end
```

The `compat53.module` module does not modify the global environment,
and so it is safe to use in modules without affecting other Lua files.
It is supposed to be set as the current environment (see above), i.e.
cherry picking individual functions from this module is expressly
*not* supported!). Not all features are available when using this
module (e.g. yieldable (x)pcall support, string/file methods, etc.),
so it is recommended to use plain `require("compat53")` whenever
possible.

### C code

There are two ways of adding the C API compatibility functions/macros to
your project:
* If `COMPAT53_PREFIX` is *not* `#define`d, `compat-5.3.h` `#include`s
  `compat-5.3.c`, and all functions are made `static`. You don't have to
  compile/link/add `compat-5.3.c` yourself. This is useful for one-file
  projects.
* If `COMPAT53_PREFIX` is `#define`d, all exported functions are renamed
  behind the scenes using this prefix to avoid linker conflicts with other
  code using this package. This doesn't change the way you call the
  compatibility functions in your code. You have to compile and link
  `compat-5.3.c` to your project yourself. You can change the way the
  functions are exported using the `COMPAT53_API` macro (e.g. if you need
  some `__declspec` magic). While it is technically possible to use
  the "lua" prefix (and it looks better in the debugger), this is
  discouraged because LuaJIT has started to implement its own Lua 5.2+
  C API functions, and with the "lua" prefix you'd violate the
  one-definition rule with recent LuaJIT versions.

## What's implemented

### Lua

* the `utf8` module backported from the Lua 5.3 sources
* `string.pack`, `string.packsize`, and `string.unpack` from the Lua
  5.3 sources or from the `struct` module. (`struct` is not 100%
  compatible to Lua 5.3's string packing!) (See [here][4])
* `math.maxinteger` and `math.mininteger`, `math.tointeger`, `math.type`,
  and `math.ult` (see [here][5])
* `assert` accepts non-string error messages
* `ipairs` respects `__index` metamethod
* `table.move`
* `table` library respects metamethods

For Lua 5.1 additionally:
* `load` and `loadfile` accept `mode` and `env` parameters
* `table.pack` and `table.unpack`
* string patterns may contain embedded zeros (but see [here][6])
* `string.rep` accepts `sep` argument
* `string.format` calls `tostring` on arguments for `%s`
* `math.log` accepts base argument
* `xpcall` takes additional arguments
* `pcall` and `xpcall` can execute functions that yield (see
  [here][22] for a possible problem with `coroutine.running`)
* `pairs` respects `__pairs` metamethod (see [here][7])
* `rawlen` (but `#` still doesn't respect `__len` for tables)
* `package.searchers` as alias for `package.loaders`
* `package.searchpath` (see [here][8])
* `coroutine` functions dealing with the main coroutine (see
  [here][22] for a possible problem with `coroutine.running`)
* `coroutine.create` accepts functions written in C
* return code of `os.execute` (see [here][9])
* `io.write` and `file:write` return file handle
* `io.lines` and `file:lines` accept format arguments (like `io.read`)
  (see [here][10] and [here][11])
* `debug.setmetatable` returns object
* `debug.getuservalue` (see [here][12])
* `debug.setuservalue` (see [here][13])

### C

* `lua_KContext` (see [here][14])
* `lua_KFunction` (see [here][14])
* `lua_dump` (extra `strip` parameter, ignored, see [here][15])
* `lua_getextraspace` (limited compatibilitiy, see [here][24])
* `lua_getfield` (return value)
* `lua_geti` and `lua_seti`
* `lua_getglobal` (return value)
* `lua_getmetafield` (return value)
* `lua_gettable` (return value)
* `lua_getuservalue` (limited compatibility, see [here][16])
* `lua_setuservalue` (limited compatibility, see [here][17])
* `lua_isinteger`
* `lua_numbertointeger`
* `lua_callk` and `lua_pcallk` (limited compatibility, see [here][14])
* `lua_resume`
* `lua_rawget` and `lua_rawgeti` (return values)
* `lua_rawgetp` and `lua_rawsetp`
* `luaL_requiref` (now checks `package.loaded` first)
* `lua_rotate`
* `lua_stringtonumber` (see [here][18])

For Lua 5.1 additionally:
* `LUA_OK`
* `LUA_ERRGCMM`
* `LUA_OP*` macros for `lua_arith` and `lua_compare`
* `LUA_FILEHANDLE`
* `lua_Unsigned`
* `luaL_Stream` (limited compatibility, see [here][19])
* `lua_absindex`
* `lua_arith` (see [here][20])
* `lua_compare`
* `lua_len`, `lua_rawlen`, and `luaL_len`
* `lua_load` (mode argument)
* `lua_pushstring`, `lua_pushlstring` (return value)
* `lua_copy`
* `lua_pushglobaltable`
* `luaL_testudata`
* `luaL_setfuncs`, `luaL_newlibtable`, and `luaL_newlib`
* `luaL_setmetatable`
* `luaL_getsubtable`
* `luaL_traceback`
* `luaL_execresult`
* `luaL_fileresult`
* `luaL_loadbufferx`
* `luaL_loadfilex`
* `luaL_checkversion` (with empty body, only to avoid compile errors,
  see [here][21])
* `luaL_tolstring`
* `luaL_buffinitsize`, `luaL_prepbuffsize`, and `luaL_pushresultsize`
  (see [here][22])
* `lua_pushunsigned`, `lua_tounsignedx`, `lua_tounsigned`,
  `luaL_checkunsigned`, `luaL_optunsigned`, if
  `LUA_COMPAT_APIINTCASTS` is defined.

## What's not implemented

* bit operators
* integer division operator
* utf8 escape sequences
* 64 bit integers
* `coroutine.isyieldable`
* Lua 5.1: `_ENV`, `goto`, labels, ephemeron tables, etc. See
  [`lua-compat-5.2`][2] for a detailed list.
* the following C API functions/macros:
  * `lua_isyieldable`
  * `lua_arith` (new operators missing)
  * `lua_push(v)fstring` (new formats missing)
  * `lua_upvalueid` (5.1)
  * `lua_upvaluejoin` (5.1)
  * `lua_version` (5.1)
  * `lua_yieldk` (5.1)

## See also

* For Lua-5.2-style APIs under Lua 5.1, see [lua-compat-5.2][2],
  which also is the basis for most of the code in this project.
* For Lua-5.1-style APIs under Lua 5.0, see [Compat-5.1][3]

## Credits

This package contains code written by:

* [The Lua Team](http://www.lua.org)
* Philipp Janda ([@siffiejoe](http://github.com/siffiejoe))
* Tomás Guisasola Gorham ([@tomasguisasola](http://github.com/tomasguisasola))
* Hisham Muhammad ([@hishamhm](http://github.com/hishamhm))
* Renato Maia ([@renatomaia](http://github.com/renatomaia))
* [@ThePhD](http://github.com/ThePhD)
* [@Daurnimator](http://github.com/Daurnimator)


  [1]: http://www.inf.puc-rio.br/~roberto/struct/
  [2]: http://github.com/lunarmodules/lua-compat-5.2/
  [3]: http://lunarmodules.org/compat/
  [4]: https://github.com/lunarmodules/lua-compat-5.3/wiki/string_packing
  [5]: https://github.com/lunarmodules/lua-compat-5.3/wiki/math.type
  [6]: https://github.com/lunarmodules/lua-compat-5.3/wiki/pattern_matching
  [7]: https://github.com/lunarmodules/lua-compat-5.3/wiki/pairs
  [8]: https://github.com/lunarmodules/lua-compat-5.3/wiki/package.searchpath
  [9]: https://github.com/lunarmodules/lua-compat-5.3/wiki/os.execute
  [10]: https://github.com/lunarmodules/lua-compat-5.3/wiki/io.lines
  [11]: https://github.com/lunarmodules/lua-compat-5.3/wiki/file.lines
  [12]: https://github.com/lunarmodules/lua-compat-5.3/wiki/debug.getuservalue
  [13]: https://github.com/lunarmodules/lua-compat-5.3/wiki/debug.setuservalue
  [14]: https://github.com/lunarmodules/lua-compat-5.3/wiki/yieldable_c_functions
  [15]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_dump
  [16]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_getuservalue
  [17]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_setuservalue
  [18]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_stringtonumber
  [19]: https://github.com/lunarmodules/lua-compat-5.3/wiki/luaL_Stream
  [20]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_arith
  [21]: https://github.com/lunarmodules/lua-compat-5.3/wiki/luaL_checkversion
  [22]: https://github.com/lunarmodules/lua-compat-5.3/wiki/luaL_Buffer
  [23]: https://github.com/lunarmodules/lua-compat-5.3/wiki/coroutine.running
  [24]: https://github.com/lunarmodules/lua-compat-5.3/wiki/lua_getextraspace

