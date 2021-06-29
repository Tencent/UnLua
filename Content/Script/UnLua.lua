local rawget = rawget
local rawset = rawset
local type = type
local getmetatable = getmetatable
local setmetatable = setmetatable
local require = require
local str_sub = string.sub

local GetUProperty = GetUProperty
local SetUProperty = SetUProperty
local RegisterClass = RegisterClass
local RegisterEnum = RegisterEnum
local print = UEPrint

_NotExist = _NotExist or {}
local NotExist = _NotExist

local function Index(t, k)
	local mt = getmetatable(t)
	local super = mt
	while super do
		local v = rawget(super, k)
        if v~= nil then
            if rawequal(v, NotExist) then
                return nil
            end
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

local function global_index(t, k)
	if type(k) == "string" then
		local s = str_sub(k, 1, 1)
		if s == "U" or s == "A" or s == "F" then
			RegisterClass(k)
		elseif s == "E" then
			RegisterEnum(k)
		end
	end
	return rawget(t, k)
end

if WITH_UE4_NAMESPACE then
	print("WITH_UE4_NAMESPACE==true");
else
	local global_mt = {}
	global_mt.__index = global_index
	setmetatable(_G, global_mt)
	UE4 = _G

	print("WITH_UE4_NAMESPACE==false");
end

_G.print = print
_G.Index = Index
_G.NewIndex = NewIndex
_G.Class = Class