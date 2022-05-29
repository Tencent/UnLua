local rawget = rawget
local rawset = rawset
local type = type
local getmetatable = getmetatable
local require = require

local GetUProperty = GetUProperty
local SetUProperty = SetUProperty

local NotExist = {}

local function Index(t, k)
	local mt = getmetatable(t)
	local super = mt
	while super do
		local v = rawget(super, k)
		if v ~= nil and not rawequal(v, NotExist) then
			rawset(t, k, v)
			return v
		end
		super = rawget(super, "Super")
	end

	local p = mt[k]
	if p ~= nil then
		if type(p) == "userdata" then
			return GetUProperty(t, p)
		elseif type(p) == "function" then
			rawset(t, k, p)
		elseif rawequal(p, NotExist) then
			return nil
		end
	else
		rawset(mt, k, NotExist)
	end

	return p
end

local function NewIndex(t, k, v)
	local mt = getmetatable(t)
	local p = mt[k]
	if type(p) == "userdata" then
		return SetUProperty(t, p, v)
	end
	rawset(t, k, v)
end

local function Class(super_name)
	local super_class = nil
	if super_name ~= nil then
		super_class = require(super_name)
	end

	local new_class = {}
	new_class.__index = Index
	new_class.__newindex = NewIndex
	new_class.Super = super_class

  return new_class
end

_G.Class = Class