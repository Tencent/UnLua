#!/usr/bin/env lua

local bit32 = require("bit32")


assert(bit32.bnot(0) == 2^32-1)
assert(bit32.bnot(-1) == 0)
assert(bit32.band(1, 3, 5) == 1)
assert(bit32.bor(1, 3, 5) == 7)

