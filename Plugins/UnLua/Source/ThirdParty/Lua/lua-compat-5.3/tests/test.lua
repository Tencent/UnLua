#!/usr/bin/env lua

local F, tproxy, writefile, noprint, ___
do
  local type, unpack = type, table.unpack or unpack
  local assert, io = assert, io
  function F(...)
    local args, n = { ... }, select('#', ...)
    for i = 1, n do
      local t = type(args[i])
      if t ~= "string" and t ~= "number" and t ~= "boolean" then
        args[i] = t
      end
    end
    return unpack(args, 1, n)
  end
  function tproxy(t)
    return setmetatable({}, {
      __index = t,
      __newindex = t,
      __len = function() return #t end,
    }), t
  end
  function writefile(name, contents, bin)
    local f = assert(io.open(name, bin and "wb" or "w"))
    f:write(contents)
    f:close()
  end
  function noprint() end
  local sep = ("="):rep(70)
  function ___()
    print(sep)
  end
end

local V = _VERSION:gsub("^.*(%d+)%.(%d+)$", "%1%2")
if jit then V = "jit" end

local mode = "global"
if arg[1] == "module" then
  mode = "module"
end
local self = arg[0]

package.path = "../?.lua;../?/init.lua"
package.cpath = "./?-"..V..".so;./?-"..V..".dll;./?.so;./?.dll"
if mode == "module" then
  print("testing Lua API using `compat53.module` ...")
  _ENV = require("compat53.module")
  if setfenv then setfenv(1, _ENV) end
else
  print("testing Lua API using `compat53` ...")
  require("compat53")
end


___''
do
  print("assert", F(pcall(assert, false)))
  print("assert", F(pcall(assert, false, nil)))
  print("assert", F(pcall(assert, false, "error msg")))
  print("assert", F(pcall(assert, nil, {})))
  print("assert", F(pcall(assert, 1, 2, 3)))
end


___''
do
  local t = setmetatable({}, { __index = { 1, false, "three" } })
  for i,v in ipairs(t) do
    print("ipairs", i, v)
  end
end


___''
do
  local p, t = tproxy{ "a", "b", "c" }
  print("table.concat", table.concat(p))
  print("table.concat", table.concat(p, ",", 2))
  print("table.concat", table.concat(p, ".", 1, 2))
  print("table.concat", table.concat(t))
  print("table.concat", table.concat(t, ",", 2))
  print("table.concat", table.concat(t, ".", 1, 2))
end


___''
do
  local p, t = tproxy{ "a", "b", "c" }
  table.insert(p, "d")
  print("table.insert", next(p), t[4])
  table.insert(p, 1, "z")
  print("table.insert", next(p),  t[1], t[2])
  table.insert(p, 2, "y")
  print("table.insert", next(p), t[1], t[2], p[3])
  t = { "a", "b", "c" }
  table.insert(t, "d")
  print("table.insert", t[1], t[2], t[3], t[4])
  table.insert(t, 1, "z")
  print("table.insert", t[1], t[2], t[3], t[4], t[5])
  table.insert(t, 2, "y")
  print("table.insert", t[1], t[2], t[3], t[4], t[5])
end


___''
do
  local ps, s = tproxy{ "a", "b", "c", "d" }
  local pd, d = tproxy{ "A", "B", "C", "D" }
  table.move(ps, 1, 4, 1, pd)
  print("table.move", next(pd), d[1], d[2], d[3], d[4])
  pd, d = tproxy{ "A", "B", "C", "D" }
  table.move(ps, 2, 4, 1, pd)
  print("table.move", next(pd), d[1], d[2], d[3], d[4])
  pd, d = tproxy{ "A", "B", "C", "D" }
  table.move(ps, 2, 3, 4, pd)
  print("table.move", next(pd), d[1], d[2], d[3], d[4], d[5])
  table.move(ps, 2, 4, 1)
  print("table.move", next(ps), s[1], s[2], s[3], s[4])
  ps, s = tproxy{ "a", "b", "c", "d" }
  table.move(ps, 2, 3, 4)
  print("table.move", next(ps), s[1], s[2], s[3], s[4], s[5])
  s = { "a", "b", "c", "d" }
  d = { "A", "B", "C", "D" }
  table.move(s, 1, 4, 1, d)
  print("table.move", d[1], d[2], d[3], d[4])
  d = { "A", "B", "C", "D" }
  table.move(s, 2, 4, 1, d)
  print("table.move", d[1], d[2], d[3], d[4])
  d = { "A", "B", "C", "D" }
  table.move(s, 2, 3, 4, d)
  print("table.move", d[1], d[2], d[3], d[4], d[5])
  table.move(s, 2, 4, 1)
  print("table.move", s[1], s[2], s[3], s[4])
  s = { "a", "b", "c", "d" }
  table.move(s, 2, 3, 4)
  print("table.move", s[1], s[2], s[3], s[4], s[5])
end


___''
do
  local p, t = tproxy{ "a", "b", "c", "d", "e" }
  print("table.remove", table.remove(p))
  print("table.remove", next(p), t[1], t[2], t[3], t[4], t[5])
  print("table.remove", table.remove(p, 1))
  print("table.remove", next(p), t[1], t[2], t[3], t[4])
  print("table.remove", table.remove(p, 2))
  print("table.remove", next(p), t[1], t[2], t[3])
  print("table.remove", table.remove(p, 3))
  print("table.remove", next(p), t[1], t[2], t[3])
  p, t = tproxy{}
  print("table.remove", table.remove(p))
  print("table.remove", next(p), next(t))
  t = { "a", "b", "c", "d", "e" }
  print("table.remove", table.remove(t))
  print("table.remove", t[1], t[2], t[3], t[4], t[5])
  print("table.remove", table.remove(t, 1))
  print("table.remove", t[1], t[2], t[3], t[4])
  print("table.remove", table.remove(t, 2))
  print("table.remove", t[1], t[2], t[3])
  print("table.remove", table.remove(t, 3))
  print("table.remove", t[1], t[2], t[3])
  t = {}
  print("table.remove", table.remove(t))
  print("table.remove", next(t))
end

___''
do
  local p, t = tproxy{ 3, 1, 5, 2, 8, 5, 2, 9, 7, 4 }
  table.sort(p)
  print("table.sort", next(p))
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  table.sort(p)
  print("table.sort", next(p))
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  p, t = tproxy{ 9, 8, 7, 6, 5, 4, 3, 2, 1 }
  table.sort(p)
  print("table.sort", next(p))
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  table.sort(p, function(a, b) return a > b end)
  print("table.sort", next(p))
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  p, t = tproxy{ 1, 1, 1, 1, 1 }
  print("table.sort", next(p))
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  t = { 3, 1, 5, 2, 8, 5, 2, 9, 7, 4 }
  table.sort(t)
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
  table.sort(t, function(a, b) return a > b end)
  for i,v in ipairs(t) do
    print("table.sort", i, v)
  end
end


___''
do
  local p, t = tproxy{ "a", "b", "c" }
  print("table.unpack", table.unpack(p))
  print("table.unpack", table.unpack(p, 2))
  print("table.unpack", table.unpack(p, 1, 2))
  print("table.unpack", table.unpack(t))
  print("table.unpack", table.unpack(t, 2))
  print("table.unpack", table.unpack(t, 1, 2))
end


___''
print("math.maxinteger", math.maxinteger+1 > math.maxinteger)
print("math.mininteger", math.mininteger-1 < math.mininteger)


___''
print("math.tointeger", math.tointeger(0))
print("math.tointeger", math.tointeger(math.pi))
print("math.tointeger", math.tointeger("hello"))
print("math.tointeger", math.tointeger(math.maxinteger+2.0))
print("math.tointeger", math.tointeger(math.mininteger*2.0))


___''
print("math.type", math.type(0))
print("math.type", math.type(math.pi))
print("math.type", math.type("hello"))


___''
print("math.ult", math.ult(1, 2), math.ult(2, 1))
print("math.ult", math.ult(-1, 2), math.ult(2, -1))
print("math.ult", math.ult(-1, -2), math.ult(-2, -1))
print("math.ult", pcall(math.ult, "x", 2))
print("math.ult", pcall(math.ult, 1, 2.1))
___''


if utf8.len then
  local unpack = table.unpack or unpack
  local function utf8rt(s)
    local t = { utf8.codepoint(s, 1, #s) }
    local ps, cs = {}, {}
    for p,c in utf8.codes(s) do
      ps[#ps+1], cs[#cs+1] = p, c
    end
    print("utf8.codes", unpack(ps))
    print("utf8.codes", unpack(cs))
    print("utf8.codepoint", unpack(t))
    print("utf8.len", utf8.len(s), #t, #s)
    print("utf8.char", utf8.char(unpack(t)))
  end
  utf8rt("äöüßÄÖÜ")
  utf8rt("abcdefg")
  ___''
  local s = "äöüßÄÖÜ"
  print("utf8.offset", utf8.offset(s, 1, 1))
  print("utf8.offset", utf8.offset(s, 2, 1))
  print("utf8.offset", utf8.offset(s, 3, 1))
  print("utf8.offset", pcall(utf8.offset, s, 3, 2))
  print("utf8.offset", utf8.offset(s, 3, 3))
  print("utf8.offset", utf8.offset(s, -1, 7))
  print("utf8.offset", utf8.offset(s, -2, 7))
  print("utf8.offset", utf8.offset(s, -3, 7))
  print("utf8.offset", utf8.offset(s, -1))
  ___''
else
  print("XXX: utf8 module not available")
end


if string.pack then
  local format = "bBhHlLjJdc3z"
  local s = string.pack(format, -128, 255, -32768, 65535, -2147483648, 4294967295, -32768, 65536, 1.25, "abc", "defgh")
  print("string.unpack", string.unpack(format, s))
  ___''
else
  print("XXX: string packing not available")
end


print("testing Lua API for Lua 5.1 ...")

___''
print("debug.getuservalue()", F(debug.getuservalue(false)))
print("debug.setuservalue()", pcall(function()
  debug.setuservalue(false, {})
end))
print("debug.setmetatable()", F(debug.setmetatable({}, {})))


___''
do
   local t = setmetatable({}, {
      __pairs = function() return pairs({ a = "a" }) end,
   })
   for k,v in pairs(t) do
      print("pairs()", k, v)
   end
end


___''
do
   local code = "print('hello world')\n"
   local badcode = "print('blub\n"
   print("load()", pcall(function() load(true) end))
   print("load()", F(load(badcode)))
   print("load()", F(load(code)))
   print("load()", F(load(code, "[L]")))
   print("load()", F(load(code, "[L]", "b")))
   print("load()", F(load(code, "[L]", "t")))
   print("load()", F(load(code, "[L]", "bt")))
   local f = load(code, "[L]", "bt", {})
   print("load()", pcall(f))
   f = load(code, "[L]", "bt", { print = noprint })
   print("load()", pcall(f))
   local bytecode = string.dump(f)
   print("load()", F(load(bytecode)))
   print("load()", F(load(bytecode, "[L]")))
   print("load()", F(load(bytecode, "[L]", "b")))
   print("load()", F(load(bytecode, "[L]", "t")))
   print("load()", F(load(bytecode, "[L]", "bt")))
   f = load(bytecode, "[L]", "bt", {})
   print("load()", pcall(f))
   f = load(bytecode, "[L]", "bt", { print = noprint })
   print("load()", pcall(f))
   local function make_loader(code)
      local mid = math.floor( #code/2 )
      local array = { code:sub(1, mid), code:sub(mid+1) }
      local i = 0
      return function()
         i = i + 1
         return array[i]
      end
   end
   print("load()", F(load(make_loader(badcode))))
   print("load()", F(load(make_loader(code))))
   print("load()", F(load(make_loader(code), "[L]")))
   print("load()", F(load(make_loader(code), "[L]", "b")))
   print("load()", F(load(make_loader(code), "[L]", "t")))
   print("load()", F(load(make_loader(code), "[L]", "bt")))
   f = load(make_loader(code), "[L]", "bt", {})
   print("load()", pcall(f))
   f = load(make_loader(code), "[L]", "bt", { print = noprint })
   print("load()", pcall(f))
   print("load()", F(load(make_loader(bytecode))))
   print("load()", F(load(make_loader(bytecode), "[L]")))
   print("load()", F(load(make_loader(bytecode), "[L]", "b")))
   print("load()", F(load(make_loader(bytecode), "[L]", "t")))
   print("load()", F(load(make_loader(bytecode), "[L]", "bt")))
   f = load(make_loader(bytecode), "[L]", "bt", {})
   print("load()", pcall(f))
   f = load(make_loader(bytecode), "[L]", "bt", { print = noprint })
   print("load()", pcall(f))
   writefile("good.lua", code)
   writefile("bad.lua", badcode)
   writefile("good.luac", bytecode, true)
   print("loadfile()", F(loadfile("bad.lua")))
   print("loadfile()", F(loadfile("good.lua")))
   print("loadfile()", F(loadfile("good.lua", "b")))
   print("loadfile()", F(loadfile("good.lua", "t")))
   print("loadfile()", F(loadfile("good.lua", "bt")))
   f = loadfile("good.lua", "bt", {})
   print("loadfile()", pcall(f))
   f = loadfile("good.lua", "bt", { print = noprint })
   print("loadfile()", pcall(f))
   print("loadfile()", F(loadfile("good.luac")))
   print("loadfile()", F(loadfile("good.luac", "b")))
   print("loadfile()", F(loadfile("good.luac", "t")))
   print("loadfile()", F(loadfile("good.luac", "bt")))
   f = loadfile("good.luac", "bt", {})
   print("loadfile()", pcall(f))
   f = loadfile("good.luac", "bt", { print = noprint })
   print("loadfile()", pcall(f))
   os.remove("good.lua")
   os.remove("bad.lua")
   os.remove("good.luac")
end


___''
do
   local function func(throw)
      if throw then
         error("argh")
      else
         return 1, 2, 3
      end
   end
   local function tb(err) return "|"..err.."|" end
   print("xpcall()", xpcall(func, debug.traceback, false))
   print("xpcall()", xpcall(func, debug.traceback, true))
   print("xpcall()", xpcall(func, tb, true))
   if mode ~= "module" then
     local function func2(cb)
       print("xpcall()", xpcall(cb, debug.traceback, "str"))
     end
     local function func3(cb)
       print("pcall()", pcall(cb, "str"))
     end
     local function cb(arg)
        coroutine.yield(2)
        return arg
     end
     local c = coroutine.wrap(func2)
     print("xpcall()", c(cb))
     print("xpcall()", c())
     local c = coroutine.wrap(func3)
     print("pcall()", c(cb))
     print("pcall()", c())
   end
end


___''
do
   local t = setmetatable({ 1 }, { __len = function() return 5 end })
   print("rawlen()", rawlen(t), rawlen("123"))
end


___''
print("os.execute()", os.execute("exit 1"))
io.flush()
print("os.execute()", os.execute("echo 'hello world!'"))
io.flush()
print("os.execute()", os.execute("no_such_file"))


___''
do
   local t = table.pack("a", nil, "b", nil)
   print("table.(un)pack()", t.n, table.unpack(t, 1, t.n))
end


___''
do
   print("coroutine.running()", F(coroutine.wrap(function()
      return coroutine.running()
   end)()))
   print("coroutine.running()", F(coroutine.running()))
   local main_co, co1, co2 = coroutine.running()
   -- coroutine.yield
   if mode ~= "module" then
     print("coroutine.yield()", pcall(function()
        coroutine.yield(1, 2, 3)
     end))
   end
   print("coroutine.yield()", coroutine.wrap(function()
      coroutine.yield(1, 2, 3)
   end)())
   print("coroutine.resume()", coroutine.resume(main_co, 1, 2, 3))
   co1 = coroutine.create(function(a, b, c)
      print("coroutine.resume()", a, b, c)
      return a, b, c
   end)
   print("coroutine.resume()", coroutine.resume(co1, 1, 2, 3))
   co1 = coroutine.create(function()
      print("coroutine.status()", "[co1] main is", coroutine.status(main_co))
      print("coroutine.status()", "[co1] co2 is", coroutine.status(co2))
   end)
   co2 = coroutine.create(function()
      print("coroutine.status()", "[co2] main is", coroutine.status(main_co))
      print("coroutine.status()", "[co2] co2 is", coroutine.status(co2))
      coroutine.yield()
      coroutine.resume(co1)
   end)
   print("coroutine.status()", coroutine.status(main_co))
   print("coroutine.status()", coroutine.status(co2))
   coroutine.resume(co2)
   print("coroutine.status()", F(coroutine.status(co2)))
   coroutine.resume(co2)
   print("coroutine.status()", F(coroutine.status(co2)))
end


___''
print("math.log()", math.log(1000))
print("math.log()", math.log(1000, 10))


___''
do
   local path, prefix = "./?.lua;?/init.lua;../?.lua", "package.searchpath()"
   print(prefix, package.searchpath("no.such.module", path))
   print(prefix, package.searchpath("no.such.module", ""))
   print(prefix, package.searchpath("compat53", path))
   print(prefix, package.searchpath("no:such:module", path, ":", "|"))
end


___''
if mode ~= "module" then
   local function mod_func() return {} end
   local function my_searcher(name)
      if name == "my.module" then
         print("package.searchers", "my.module found")
         return mod_func
      end
   end
   local function my_searcher2(name)
      if name == "my.module" then
         print("package.searchers", "my.module found 2")
         return mod_func
      end
   end
   table.insert(package.searchers, my_searcher)
   require("my.module")
   package.loaded["my.module"] = nil
   local new_s = { my_searcher2 }
   for i,f in ipairs(package.searchers) do
      new_s[i+1] = f
   end
   package.searchers = new_s
   require("my.module")
end


___''
do
   print("string.find()", string.find("abc\0abc\0abc", "[^a\0]+"))
   print("string.find()", string.find("abc\0abc\0abc", "%w+\0", 5))
   for x in string.gmatch("abc\0def\0ghi", "[^\0]+") do
      print("string.gmatch()", x)
   end
   for x in string.gmatch("abc\0def\0ghi", "%w*\0") do
      print("string.gmatch()", #x)
   end
   print("string.gsub()", string.gsub("abc\0def\0ghi", "[\0]", "X"))
   print("string.gsub()", string.gsub("abc\0def\0ghi", "%w*\0", "X"))
   print("string.gsub()", string.gsub("abc\0def\0ghi", "%A", "X"))
   print("string.match()", string.match("abc\0abc\0abc", "([^\0a]+)"))
   print("string.match()", #string.match("abc\0abc\0abc", ".*\0"))
   print("string.rep()", string.rep("a", 0))
   print("string.rep()", string.rep("b", 1))
   print("string.rep()", string.rep("c", 4))
   print("string.rep()", string.rep("a", 0, "|"))
   print("string.rep()", string.rep("b", 1, "|"))
   print("string.rep()", string.rep("c", 4, "|"))
   local _tostring = tostring
   function tostring(v)
      if type(v) == "number" then
         return "(".._tostring(v)..")"
      else
         return _tostring(v)
      end
   end
   print("string.format()", string.format("%q", "\"\\\0000\0010\002\r\n0\t0\""))
   print("string.format()", string.format("%12.3fx%%sxx%.6s", 3.1, {}))
   print("string.format()", string.format("%-3f %%%s %%s", 3.1, true))
   print("string.format()", string.format("% 3.2g %%d %%%s", 3.1, nil))
   print("string.format()", string.format("%+3d %%d %%%%%10.6s", 3, io.stdout))
   print("string.format()", pcall(function()
      print("string.format()", string.format("%d %%s", {}))
   end))
   tostring = _tostring
end


___''
do
   print("io.write()", io.type(io.write("hello world\n")))
   local f = assert(io.tmpfile())
   print("file:write()", io.type(f:write("hello world\n")))
   f:close()
end


___''
do
   writefile("data.txt", "123 18.8 hello world\ni'm here\n")
   io.input("data.txt")
   print("io.read()", io.read("*n", "*number", "*l", "*a"))
   io.input("data.txt")
   print("io.read()", io.read("n", "number", "l", "a"))
   io.input(io.stdin)
   if mode ~= "module" then
     local f = assert(io.open("data.txt", "r"))
     print("file:read()", f:read("*n", "*number", "*l", "*a"))
     f:close()
     f = assert(io.open("data.txt", "r"))
     print("file:read()", f:read("n", "number", "l", "a"))
     f:close()
   end
   os.remove("data.txt")
end


___''
do
   writefile("data.txt", "123 18.8 hello world\ni'm here\n")
   for a,b in io.lines(self, 2, "*l") do
      print("io.lines()", a, b)
      break
   end
   for l in io.lines(self) do
      print("io.lines()", l)
      break
   end
   for n1,n2,rest in io.lines("data.txt", "*n", "n", "*a") do
      print("io.lines()", n1, n2, rest)
   end
   for l in io.lines("data.txt") do
      print("io.lines()", l)
   end
   print("io.lines()", pcall(function()
      for l in io.lines("data.txt", "*x") do print(l) end
   end))
   print("io.lines()", pcall(function()
      for l in io.lines("no_such_file.txt") do print(l) end
   end))
   if mode ~= "module" then
     local f = assert(io.open(self, "r"))
     for a,b in f:lines(2, "*l") do
        print("file:lines()", a, b)
        break
     end
     f:close()
     f = assert(io.open("data.txt", "r"))
     for n1,n2,rest in f:lines("*n", "n", "*a") do
        print("file:lines()", n1, n2, rest)
     end
     f:close()
     f = assert(io.open("data.txt", "r"))
     for l in f:lines() do
        print("file:lines()", l)
     end
     f:close()
     print("file:lines()", pcall(function()
        for l in f:lines() do print(l) end
     end))
     print("file:lines()", pcall(function()
        local f = assert(io.open("data.txt", "r"))
        for l in f:lines("*l", "*x") do print(l) end
        f:close()
     end))
   end
   os.remove("data.txt")
end
___''


print("testing C API ...")
local mod = require("testmod")
___''
print("isinteger", mod.isinteger(1))
print("isinteger", mod.isinteger(0))
print("isinteger", mod.isinteger(1234567))
print("isinteger", mod.isinteger(12.3))
print("isinteger", mod.isinteger(math.huge))
print("isinteger", mod.isinteger(math.sqrt(-1)))

___''
print("rotate", mod.rotate(1, 1, 2, 3, 4, 5, 6))
print("rotate", mod.rotate(-1, 1, 2, 3, 4, 5, 6))
print("rotate", mod.rotate(4, 1, 2, 3, 4, 5, 6))
print("rotate", mod.rotate(-4, 1, 2, 3, 4, 5, 6))

___''
print("strtonum", mod.strtonum("+123"))
print("strtonum", mod.strtonum(" 123 "))
print("strtonum", mod.strtonum("-1.23"))
print("strtonum", mod.strtonum(" 123 abc"))
print("strtonum", mod.strtonum("jkl"))

___''
local a, b, c = mod.requiref()
print("requiref", type(a), type(b), type(c),
      a.boolean, b.boolean, c.boolean,
      type(requiref1), type(requiref2), type(requiref3))

___''
local c = coroutine.wrap(function()
  mod.extraspace("uvw")
  print("getextraspace", mod.extraspace())
  coroutine.yield()
  print("getextraspace", mod.extraspace())
  coroutine.yield()
  print("getextraspace", mod.extraspace())
end)
c()
mod.extraspace("abc")
print("getextraspace", mod.extraspace())
c()
local d = coroutine.wrap(function()
  print("getextraspace", mod.extraspace())
  mod.extraspace("xyz")
  print("getextraspace", mod.extraspace())
  coroutine.yield()
  print("getextraspace", mod.extraspace())
  coroutine.yield()
  print("getextraspace", mod.extraspace())
end)
d()
print("getextraspace", mod.extraspace())
mod.extraspace("123")
c()
d()

___''
local proxy, backend = {}, {}
setmetatable(proxy, { __index = backend, __newindex = backend })
print("geti/seti", rawget(proxy, 1), rawget(backend, 1))
print("geti/seti", mod.getseti(proxy, 1))
print("geti/seti", rawget(proxy, 1), rawget(backend, 1))
print("geti/seti", mod.getseti(proxy, 1))
print("geti/seti", rawget(proxy, 1), rawget(backend, 1))

-- tests for Lua 5.1
___''
print("tonumber", mod.tonumber(12))
print("tonumber", mod.tonumber("12"))
print("tonumber", mod.tonumber("0"))
print("tonumber", mod.tonumber(false))
print("tonumber", mod.tonumber("error"))

___''
print("tointeger", mod.tointeger(12))
print("tointeger", mod.tointeger(12))
print("tointeger", mod.tointeger(12.1))
print("tointeger", mod.tointeger(12.9))
print("tointeger", mod.tointeger(-12.1))
print("tointeger", mod.tointeger(-12.9))
print("tointeger", mod.tointeger("12"))
print("tointeger", mod.tointeger("0"))
print("tointeger", mod.tointeger(math.pi))
print("tointeger", mod.tointeger(false))
print("tointeger", mod.tointeger("error"))

___''
print("len", mod.len("123"))
print("len", mod.len({ 1, 2, 3}))
print("len", pcall(mod.len, true))
local ud, meta = mod.newproxy()
meta.__len = function() return 5 end
print("len", mod.len(ud))
meta.__len = function() return true end
print("len", pcall(mod.len, ud))

___''
print("copy", mod.copy(true, "string", {}, 1))

___''
print("rawgetp/rawsetp", mod.rawxetp())
print("rawgetp/rawsetp", mod.rawxetp("I'm back"))

___''
print("globals", F(mod.globals()), mod.globals() == _G)

___''
local t = {}
print("getsubtable", F(mod.subtable(t)))
local x, msg = mod.subtable(t)
print("getsubtable", F(x, msg, x == t.xxx))

___''
print("udata", F(mod.udata()))
print("udata", mod.udata("nosuchtype"))

___''
print("uservalue", F(mod.uservalue()))

___''
print("upvalues", mod.getupvalues())

___''
print("absindex", mod.absindex("hi", true))

___''
print("arith", mod.arith(2, 1))
print("arith", mod.arith(3, 5))

___''
print("compare", mod.compare(1, 1))
print("compare", mod.compare(2, 1))
print("compare", mod.compare(1, 2))

___''
print("tolstring", mod.tolstring("string"))
local t = setmetatable({}, {
  __tostring = function(v) return "mytable" end
})
print("tolstring", mod.tolstring(t))
local t = setmetatable({}, {
  __tostring = function(v) return nil end
})
print("tolstring", pcall(mod.tolstring, t))
local ud, meta = mod.newproxy()
meta.__name = "XXX"
print("tolstring", mod.tolstring(ud):gsub(":.*$", ": yyy"))

___''
print("pushstring", mod.pushstring())

___''
print("Buffer", mod.buffer())

___''
print("execresult", mod.exec("exit 0"))
print("execresult", mod.exec("exit 1"))
print("execresult", mod.exec("exit 25"))

___''
do
  local bin = string.dump(function() end)
  local modes = { "t", "b", "bt" }
  local codes = {
    "", "return true", bin, "invalidsource", "\27invalidbinary"
  }
  for _,m in ipairs(modes) do
    for i,c in ipairs(codes) do
      print("loadbufferx", m, i, F(mod.loadstring(c, m)))
    end
  end

  ___''
  local bom = "\239\187\191"
  local shebang = "#!/usr/bin/env lua\n"
  codes[#codes+1] = bom .. shebang .. "return true"
  codes[#codes+1] = bom .. shebang .. bin
  codes[#codes+1] = bom .. shebang .. "invalidsource"
  codes[#codes+1] = bom .. shebang .. "\027invalidbinary"
  for _,m in ipairs(modes) do
    for i,c in ipairs(codes) do
      print("loadfilex", m, i, F(mod.loadfile(c, m)))
    end
  end
end
___''

