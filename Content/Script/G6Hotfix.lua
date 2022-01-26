------------------------------------------
--- HotFix - 运行时HotFix支持 参考云风方案
--- 替换运行环境中的函数，保持upvalue和运行时的table
--- 为开发期设计，尽量替换。最差的结果就是重新启动
--- 原表改变，类型改变 后逻辑由业务保证正确
--- 基于lua 5.3，重定向require，使用env加载实现沙盒
--- 提供一系列回调注册，可以在hotfix前后做处理
------------------------------------------
--在Module中如果定义此标记，则对此module进行reload操作
--like local M = { _Hot_All_Reload_Mark_ = true }
local HotAllReloadMark = "_Hot_All_Reload_Mark_"

---@class GHotFix
local G6HotFix = {
	--此标记位决定在HotFix后，是将新的table中的函数赋给老的table来保持状态
	--还是使用新的table替换全局中老的table
	UseNewModuleWhenHotfix = false,
	--是否删除旧table中有，新table没有的值
	DelOldAddedValue = false,
	m_Loaded = setmetatable({}, { __mode = "v" }),
	HotFixCount = 0,
	WhiteList = {}
}

function G6HotFix.print(...)
	print("GHotFix DEBUG : ", ...)
end

local table = table
local debug = debug
local OriRequire = require
local strexmsg
local function error_handler(err)
  print(strexmsg .. "  " .. err .. "  " .. debug.traceback())
end

--- 脚本加载的修改时间
---@type table<string, number>
local tfilesload_recordtime = {}

--- SandBox Begin ---
--- HotFix过程中使用的沙盒,HotFix时需要Init，完成后Clear
---@class SandBox
local SandBox = {
	tloaded_dummy = setmetatable({}, { __mode = "kv" }),
	bIsHotfixing = false
}

--- 沙盒初始化
---@param loadedlistname table
---@param loadedlistmodule table
function SandBox.Init(loadedlistname, loadedlistmodule)
	SandBox.bIsHotfixing = true

	SandBox.tloaded_dummy = setmetatable({}, { __mode = "kv" })

	if loadedlistname then
		for index, name in ipairs(loadedlistname) do
			local mod = loadedlistmodule[index]
			SandBox.tloaded_dummy[name] = mod
			SandBox.tloaded_dummy[mod] = name
		end
	end
end

--- 清理沙盒
function SandBox.Clear()
	SandBox.bIsHotfixing = false
	SandBox.tloaded_dummy = setmetatable({}, { __mode = "kv" })
end

--- 沙盒所需的Require方法
---@param filename string
function SandBox.Requrie(filename)
	local fnv = nil
	if SandBox.bIsHotfixing then
		if SandBox.tloaded_dummy[filename] ~= nil then
			return	SandBox.tloaded_dummy[filename]
	   	end
	end
	fnv = G6HotFix.RequireFile(filename)
	if SandBox.bIsHotfixing then
		--print("SandBox.Requrie ", fnv)
		SandBox.tloaded_dummy[filename] = fnv
		SandBox.tloaded_dummy[fnv] = filename
	end
	return fnv
end

function SandBox.IsDummy(obj)
	return SandBox.tloaded_dummy[obj] ~= nil
end

--- 文件载入的元表，使文件在沙盒内运行
local m_mtSandbox = {}
function m_mtSandbox:__index(k)
    if k == "require" then
    	return SandBox.Requrie
    end
    return _G[k]
end

local function GetClassModuleEvn(classname)
	local env = {}
	setmetatable(env, m_mtSandbox)
	--env.__clasename = classname
	return env
end

---	SandBox End ---

---@param strname string
---@param bnotprinterr boolean
---@return function, string, table
local function SelfLoadFile(strname, bnotprinterr)
	local env = GetClassModuleEvn(strname)
	local func, err = UELoadLuaFile(strname, "bt", env)
	if not func and not bnotprinterr then
		print("SelfLoadFile : loadfile failed : ", strname, err)
	end
	return func, err, env
end

--- 先检测require的Module是否存在
--- @return boolean
local function IsModuleInPackage(strname)
	if package.loaded[strname] ~= nil then
		--print("package.loaded 已包含 : " .. strname)
		return true
	end
	-- Searchers in UnLua2.1+ , see in LuaContext.cpp
	-- 1:package.preload 2:UnLua CustomLoader 3:UnLua FileSystem 4:UnLua BuiltinLibs 5:Lua loader 6:C Loader 7:all-in-one loader
	for i, loader in ipairs(package.searchers) do
		--不使用Lua loader
		if i ~= 3 and i ~= 5 then
			local f , extra = loader(strname)
			local t = type(f)
			if t == "function" then
				return true
			end
		end
	end
	return false
end

---@param strName strFileName
function GetFileModifyTime(strFileName)
    local fullName = strFileName
    if G6ENV and G6ENV._LUA_ROOT_PATH then
        fullName = G6ENV._LUA_ROOT_PATH .. strFileName
    end
    if G6lib.GetFileDateTime == nil then
        return -1
    end
    return G6lib.GetFileDateTime(fullName)
end

---@param strName string
local function SelfLoadFileAndRecord(strName)
	local print = G6HotFix.print
	if not strName then
		print(debug.traceback())
		error("load nil")
		return
	end
	--require的时候只有module名,没有文件名
	if G6HotFix.m_Loaded[strName] ~= nil then
		if package.loaded[strName] == nil then
			package.loaded[strName] = G6HotFix.m_Loaded[strName]
		end
		return G6HotFix.m_Loaded[strName]
	end

	print("SelfLoadFileAndRecord : " .. tostring(strName))
	
	local bIsRequire = false
	local filename = strName

	-- loadfile使用全路径
	if not string.find( strName, "/") then
		if IsModuleInPackage(strName) then
			print(" find " .. strName .. "in package. use ori require")
			return OriRequire(strName)
		end

		filename = string.gsub(strName, "%.", "/")
    	filename = filename..".lua"
    	bIsRequire = true  
    end
    local func, err, _fileenv = SelfLoadFile(filename, bIsRequire)
	if not func then
		local rf = OriRequire(strName)
		if rf == nil then 
			print("orir require load failed : ", strName)
		else
			print("use ori require success : ", strName)
		end
		return rf, true
	else
		strexmsg = filename
    	local _x, _newModlue = xpcall(func, error_handler)
    	if _newModlue ~= nil then
		else
			print("file : " .. filename .. " not return table. use env" )
    	 	_newModlue = _fileenv
    	end
		if G6HotFix.m_Loaded[strName] == nil then
			G6HotFix.m_Loaded[strName] = _newModlue
			package.loaded[strName] = _newModlue
			tfilesload_recordtime[strName] = GetFileModifyTime(filename)
			return _newModlue
		end
	end
	return _fileenv
end

---@param func function
---@param name string
local function FindUpvalue(func, name)
	if not func then
		return
	end
	local i = 1
	while true do
		local n,v = debug.getupvalue(func, i)
		if n == nil or name == "" then
			return
		end
		if n == name then
			return i
		end
		i = i + 1
	end
end

--- 得到table的名称，用来通知上层逻辑更新
local function GetTableName(mv)
	for k,v in pairs(G6HotFix.m_Loaded) do
		if mv == v then
			return k
		end
	end
end

local function MergeObjects(ModuleRes)
	local print = G6HotFix.print

	for _, m in ipairs(ModuleRes) do
		assert(m.old_module ~= nil)
		for index, v in ipairs(m.tValueMap) do
			for name, ValueMap in pairs(v) do
				for OldOne, NewOne in pairs(ValueMap) do
					--assert(NewOne ~= OldOne)
					if OldOne == nil and not SandBox.IsDummy(NewOne) then
						m.old_module[name] = NewOne
						if type(NewOne) == "function" then
							print("ADD NEW FUNCTION : ", GetTableName(m.old_module), name)
							--通知UNLUA，新增的函数是否需要 override BP函数
							if UnLua_OnAddNewFunction ~= nil then
								UnLua_OnAddNewFunction()
							end
						end
					elseif( type(NewOne) == "table" ) and not SandBox.IsDummy(NewOne) then
						if print then print("COPY", tostring(NewOne), tostring(OldOne)) end
						for k, nv in pairs(NewOne) do
							OldOne[k] = nv
						end
					elseif(type(NewOne) == "function") then
						local i = 1
						while true do
							local name,v = debug.getupvalue(NewOne, i)
							if name == nil or name == "" then
								break
							end
							local id = debug.upvalueid(NewOne, i)
							local uv = m.tUpvalueMap[id]
							if uv then
								if print then print("SET UV :", tostring(NewOne), name, tostring(uv.ReplaceUV)) end
								debug.setupvalue(NewOne, i, uv.ReplaceUV)
							end
						i = i + 1
						end
					end
				end			
			end
		end
	end
end

--- CallBack Begin ---
local m_tFileLoadCallBack = {}
local m_tHotfixCallBack = {}
local m_tPreHotfixCallback = {};
local m_tPostHotfixCallback = {};
--- 注册使用GHotfix时，加载文件的回调函数，可以对module上做一些处理。
--- 当使用Hotfix功能时，在Hotfix完成后，也会触发此回调函数。
--- 回调的第三个参数是bool型，标志是否为hotfix
---@param fun fun(LoadedMoudle:table, FileName:string, bHotfix:boolean) @回调函数
function G6HotFix.RegistedFileLoadCallback(fun)
	if type(fun) ~= "function" then
		print("RegistedFileLoadCallback error : need function ")
		return
	end

	m_tFileLoadCallBack[#m_tFileLoadCallBack+1] = fun
end

---Hotfix预处理回调，当使用Hotfix功能时，在Hotfix执行前，会触发此回调函数。
---@param fun fun() @回调函数
function G6HotFix.RegistedPreHotfixCallback(fun)
	if type(fun) ~= "function" then
		print("RegistedPreHotfixCallback error : need function ")
		return
	end

	m_tPreHotfixCallback[#m_tPreHotfixCallback+1] = fun
end

---Hotfix后处理回调，当使用Hotfix功能时，在Hotfix执行后，会触发此回调函数。
---@param fun fun() @回调函数
function G6HotFix.RegistedPostHotfixCallback(fun)
	if type(fun) ~= "function" then
		print("RegistedPostHotfixCallback error : need function ")
		return
	end

	m_tPostHotfixCallback[#m_tPostHotfixCallback+1] = fun
end

--- 当使用Hotfix功能时，在对此文件Hotfix后，会触发此回调函数。
---@param FileName string 关心的文件名
---@param Fun fun(HotfixCount:number, FileName:string, LoadedMoudle:table) @回调函数
function G6HotFix.RegistedHotfixCallback(FileName, Fun)
	print("RegistedHotfixCallback", FileName, Fun)
	if type(Fun) ~= "function" then
		print("RegistedHotfixCallback error : need function ")
		return
	end

	if m_tHotfixCallBack[FileName] == nil then
		m_tHotfixCallBack[FileName] = {}
	end
	m_tHotfixCallBack[FileName][#m_tHotfixCallBack[FileName] + 1] = Fun
end
--- CallBack End ---

--- 使用GHotFix的RequireFile，使文件在沙盒内加载运行，并根据需要保存初始值.
--- 如果是module的形式，返回module table.则返回值类似于require,是整个chunk返回的值
--- 如果不是以moudle的形式，则不能Hotfix
---@param strFileName string
function G6HotFix.RequireFile(strFileName)
	if package.loaded[strFileName] ~= nil then
		return package.loaded[strFileName]
	end
	
	local fevn, IsRequire = SelfLoadFileAndRecord(strFileName)
	--print("fileloadcallback :  " .. strFileName)
	if not IsRequire then
		for _, callback in ipairs(m_tFileLoadCallBack) do
			callback(fevn,strFileName, false)
		end
	end
	
	return fevn
end

--- 清除指定的已加载的Module
---@param strFileName string
function G6HotFix.ClearLoadedModuleByName(strFileName)
	G6HotFix.m_Loaded[strFileName] = nil
	tfilesload_recordtime[strFileName] = nil
	package.loaded[strFileName] = nil
end

--- 清除已加载的Module
function G6HotFix.ClearLoadedModule()
	for filename,_ in pairs(G6HotFix.m_Loaded) do
		G6HotFix.ClearLoadedModuleByName(filename)
	end
end

--- 对于已加载的，有修改的文件进行HotFix
---@param bNotPrintHotfixFile boolean
function G6HotFix.HotFixModifyFile(bNotPrintHotfixFile)
	local print = G6HotFix.print
	local needHotFixFile = {}
	local newFilename = {}
	
	if GetFileModifyTime == nil then
		print("can not get file modify time. return")
		return
	end

	for sourceFile,mtime in pairs(tfilesload_recordtime) do
		if not G6HotFix.WhiteList[sourceFile] then
			local fileFullname = sourceFile
			if not string.find( fileFullname, "/") then
				fileFullname = string.gsub(fileFullname, "%.", "/")
				fileFullname = fileFullname..".lua"
			end

			local curmtime = GetFileModifyTime(fileFullname)
			if curmtime ~= mtime then
				needHotFixFile[#needHotFixFile + 1] = sourceFile
				newFilename[#newFilename + 1] = fileFullname
				tfilesload_recordtime[sourceFile] = curmtime
			end
		end
	end
	if not bNotPrintHotfixFile then
		print("need hotfix :", TableToString(needHotFixFile))
	end
	if #needHotFixFile > 0 then
		-- 执行Hotfix
		G6HotFix.HotFixFile(needHotFixFile, newFilename)
	end
end

--- 枚举出module中所有函数的upvalue，Lua在不同的闭包间访问相同的外部local变量时，使用的是同样的upvalue
---@param moudule table
local function EnumModuleUpvalue(moudule)
	local print = G6HotFix.print
	
	local function GetFunctionUpValue(fun, resTable)
		assert(type(fun) == "function")
		local i = 1
		while true do
			local name, value = debug.getupvalue(fun, i)
			if name == nil or name == "" then
				break
			else
				if not name:find("^[_%w]") then
					error("Invalid upvalue : " .. table.concat(path, "."))
				end
				if not resTable[name] then
					resTable[name] = value
					if type(value) == "function" then
						GetFunctionUpValue(value, resTable)
					end
				end
			end
			i = i + 1
		end
	end

	local allUpvalue = {}
	for k, v in pairs(moudule) do
		if type(v) == "function" then
			GetFunctionUpValue(v, allUpvalue)
		end
	end
	return allUpvalue
end


local function EnumModule(TargetModule)
	local ModuleValue = {}
	if SandBox.IsDummy(TargetModule) then
		return ModuleValue
	end
	for k, v in pairs(TargetModule) do
		if SandBox.IsDummy(v) then
		elseif type(v) == "function" then
			table.insert(ModuleValue, {name = k, value = v})
		end
	end
	return ModuleValue
end

---@param NewModuleInfo table
---@param OldModule table
---@return table
local function MatchModule(NewModuleInfo, OldModule)
	local MapMatch = {}
	local print = G6HotFix.print
	for index, v in ipairs(NewModuleInfo) do
		local oldFun = rawget(OldModule, v.name)
		if oldFun and oldFun ~= v.value then
			table.insert(MapMatch, { [v.name] = { [oldFun] = v.value } } )
		end
	end

	return MapMatch
end

---@param ValueInfoMap table
---@param OldModuleUpvalues table
---@return table
local function MatchUpvalues(ValueInfoMap, OldModuleUpvalues)
	local print = G6HotFix.print
	local UpValueMatchMap = {}

	for index, v in ipairs(ValueInfoMap) do
		for name, ValueMap in pairs(v) do
			for OldFun, NewFun in pairs(ValueMap) do
				if( type(NewFun) == "function" ) then
					local i = 1
					while true do
						local name, uValue = debug.getupvalue(NewFun, i)
						if name == nil or name == "" then
							break
						end
						local id = debug.upvalueid(NewFun, i)
						if not UpValueMatchMap[id] then
							local ValueUV = nil
							if OldModuleUpvalues[name] ~= nil then
								ValueUV = OldModuleUpvalues[name]
							else
								--新增的upvalue
								print("ADD NEW UPVALUE : ", tostring(NewFun), name, tostring(uValue))
								--解决新增的upvalue是当前module
								if (type(uValue) == "table" or type(uValue) == "function") and ValueInfoMap[uValue] ~= nil then
									ValueUV = ValueInfoMap[uValue]
								else
									ValueUV = uValue
								end
							end
	
							if ValueUV then
								UpValueMatchMap[id] = { ReplaceUV = ValueUV }
							end
						end
						i = i + 1
					end
				end
			end
		end
	end

	return UpValueMatchMap
end

local function UpdateGlobal(ChangeValueMap)
	local print = G6HotFix.print
	local RunningState = coroutine.running()
	local Exclude = { [debug] = true, [coroutine] = true, [io] = true }
	Exclude[Exclude] = true
	Exclude[G6HotFix] = true
	Exclude[SandBox] = true
	Exclude[ChangeValueMap] = true
	Exclude[G6HotFix.UpdateModule] = true

	if G6HotFix.UseNewModuleWhenHotfix == false then
		Exclude[package] = true
		Exclude[package.loaded] = true
		Exclude[G6HotFix.m_Loaded] = true
		--print("Not Replace Package")
	end

	local ReplaceCount = 0
	local UpdateGlobalIterate

	local function UpdateRuningStack(co,level)
		local info = debug.getinfo(co, level+1, "f")
		if info == nil then
			return
		end
		local f = info.func
		info = nil
		UpdateGlobalIterate(f)
		local i = 1
		while true do
			local name, v = debug.getlocal(co, level+1, i)
			if name == nil then
				if i > 0 then
					i = -1
				else
					break
				end
			end
			local nv = ChangeValueMap[v]
			if nv then
				debug.setlocal(co, level+1, i, nv)
				UpdateGlobalIterate(nv)
			else
				UpdateGlobalIterate(v)
			end
			if i > 0 then
				i = i + 1
			else
				i = i - 1
			end
		end
		return UpdateRuningStack(co, level+1)
	end

	function UpdateGlobalIterate(root)
		if root == nil or Exclude[root] then
			return
		end
		--print("UpdateGlobalIterate : ", TableToString(root, 2))
		Exclude[root] = true
		local t = type(root)
		if t == "table" then
			local mt = getmetatable(root)
			if mt then UpdateGlobalIterate(mt) end
			local ReplaceK = {}
			for key, value in pairs(root) do
				local nv = ChangeValueMap[value]
				if nv then
					ReplaceCount = ReplaceCount + 1
					--print("ReplaceCount1 : ", key, TableToString(root), ReplaceCount)
					rawset(root, key, nv)
					UpdateGlobalIterate(nv)
				else
					UpdateGlobalIterate(value)
				end
				nv = ChangeValueMap[key]
				if nv then
					ReplaceK[key] = nv
				end
			end
			for key, value in pairs(ReplaceK) do
				ReplaceCount = ReplaceCount + 1
				--print("ReplaceCount2 : ", key, ReplaceCount)
				root[key], root[value] = nil, root[key]
				UpdateGlobalIterate(value)
			end
		elseif t == "userdata" then
			local mt = getmetatable(root)
			if mt then UpdateGlobalIterate(mt) end
			local UserValue = debug.getuservalue(root)
			if UserValue then
				local nv = ChangeValueMap[UserValue]
				if nv then
					ReplaceCount = ReplaceCount + 1
					debug.setuservalue(root, UserValue)
					UpdateGlobalIterate(nv)
				else
					UpdateGlobalIterate(UserValue)
				end
			end
		elseif t == "function" then
			local i = 1
			while true do
				local name, v = debug.getupvalue(root, i)
				if name == nil then
					break
				else
					local nv = ChangeValueMap[v]
					if nv then
						if G6HotFix.UseNewModuleWhenHotfix then
							ReplaceCount = ReplaceCount + 1
							--print("ReplaceCount3 : ", name, tostring(root), tostring(v), tostring(nv), ReplaceCount)
							debug.setupvalue(root, i, nv)
						end
						--print("UpdateGlobalIterate nv : ", name)
						UpdateGlobalIterate(nv)
					else
						--print("UpdateGlobalIterate v : ", name)
						UpdateGlobalIterate(v)
					end
				end
				i=i+1
			end
		end
	end

	UpdateRuningStack(RunningState, 2)
	UpdateGlobalIterate(_G)
	UpdateGlobalIterate(debug.getregistry())

	print("ReplaceGlobalCount : ", ReplaceCount)
end

--- 根据新的module和记录的初始module，更新对应的旧module
---@param listoldmodule table
---@param listnewmudule table
---@param listnewmuduleenv table
function G6HotFix.UpdateModule(listoldmodule, listnewmudule, listnewmuduleenv)
	local print = G6HotFix.print
	local result = {}

	print("HOT FIX START")

	for i, OldModule in ipairs(listoldmodule) do
		local NewModule = listnewmudule[i]
		local newmuduleenv = listnewmuduleenv[i]

		if NewModule[HotAllReloadMark] then
			if print then print("Module Use Reload", tostring(NewModule)) end
			local moduleres = {
				tValueMap = {},
				old_module = OldModule
			}
			table.insert(moduleres.tValueMap, { [NewModule] = {[OldModule] = NewModule } } )
			print("--------------Print ValueMap--------------")
			print(TableToString(moduleres.tValueMap, 6))
			result[i] = moduleres
		else
			local NewModuleInfo = EnumModule(NewModule)
			print("--------------Print NewModuleInfo--------------")
			print(TableToString(NewModuleInfo))
			local oldModuleUpvalues = EnumModuleUpvalue(OldModule)
			print("--------------Print OldModuleUpValue--------------")
			print(TableToString(oldModuleUpvalues, 6))
			
			local moduleres = {
				tValueMap = {},
				tUpvalueMap = {},
				old_module = OldModule,
			}

			moduleres.tValueMap = MatchModule(NewModuleInfo, OldModule)
			--keep ori table value
			if G6HotFix.UseNewModuleWhenHotfix then
				for oldk, oldv in pairs(OldModule) do
					if type(oldv) ~= "function" then
						NewModule[oldk] = oldv
					end
				end
				setmetatable(NewModule, getmetatable(OldModule))
				table.insert(moduleres.tValueMap, { [NewModule] = {[OldModule] = NewModule } } )
			else
				if G6HotFix.DelOldAddedValue then
					for oldk, okdv in pairs(OldModule) do
						if NewModule[oldk] == nil then
							OldModule[oldk] = nil
						end
					end
				end

				for newk, newv in pairs(NewModule) do
					if type(newv) == "function" then
						OldModule[newk] = newv
					end
					if OldModule[newk] == nil then
						OldModule[newk] = newv
					end
				end
			end

			print("--------------Print ValueMap--------------")
			print(TableToString(moduleres.tValueMap))

			--print(TableToString(moduleres.tValueMap))
			--MatchUpvalues(moduleres.tValueMap, moduleres.tUpvalueMap, oldModuleUpvalues) -- find match func's tUpvalueMap
			moduleres.tUpvalueMap = MatchUpvalues(moduleres.tValueMap, oldModuleUpvalues)
			print("--------------Print UVMap--------------")
			print(TableToString(moduleres.tUpvalueMap, 10))
			
			result[i] = moduleres
		end
	end

	MergeObjects(result)
	local AllValueMap = {}
	for _, rv in ipairs(result) do
		for _, v in ipairs(rv.tValueMap) do
			for name, ValueMap in pairs(v) do
				for key, value in pairs(ValueMap) do
					AllValueMap[key] = value
				end
			end
		end
	end

	print("--------------Print AllValueMap--------------")
	print(TableToString(AllValueMap))
	UpdateGlobal(AllValueMap)

	print("HOT FIX END")
	return true
end

--- 使用NewFile对OldFile进行HotFix
---@param strOldFile string
---@param strNewFile string
function G6HotFix.HotFixOneFile(strOldFile, strNewFile)
	if strNewFile == nil then
		strNewFile = strOldFile

		if not string.find( strNewFile, "/") then
			strNewFile = string.gsub(strNewFile, "%.", "/")
			strNewFile = strNewFile..".lua"
		end
	end
	if G6HotFix.m_Loaded[strOldFile] == nil then
		print("file not loaded : ", strOldFile, "new load file : ", strNewFile)
		G6HotFix.RequireFile(strNewFile)
		return true
	end

	-- 有Hotfix，触发PreHotfix
	for _, callback in ipairs(m_tPreHotfixCallback) do
		callback({strOldFile})
	end

	local tmploaded = {}
	local tmploadedmod = {}
	for k, v in pairs(G6HotFix.m_Loaded) do
		if k ~= strOldFile then
			table.insert(tmploaded, k)
			table.insert(tmploadedmod, v)
		end
	end

	SandBox.Init(tmploaded, tmploadedmod)

	local _f, err, _fileenv = SelfLoadFile(strNewFile)
	if _f ~= nil then
		strexmsg = strNewFile
		local _x, _newModlue = xpcall(_f, error_handler)
		if not _x then
			return _x, _newModlue
		end
    	if _newModlue ~= nil then
		else
			print("file : " .. strNewFile .. " not return table. use env" )
    	 	_newModlue = _fileenv
		end	
		G6HotFix.UpdateModule({G6HotFix.m_Loaded[strOldFile]}, {_newModlue}, {_fileenv})

		if G6HotFix.UseNewModuleWhenHotfix then
			--change to new or will be nil
			G6HotFix.m_Loaded[strOldFile] = _newModlue
			package.loaded[strOldFile] = _newModlue
		end
	end
	SandBox.Clear()
	if _f == nil then
		return
	end

	G6HotFix.HotFixCount = G6HotFix.HotFixCount + 1

	for _, callback in ipairs(m_tFileLoadCallBack) do
		callback(G6HotFix.m_Loaded[strOldFile], strOldFile, true)
	end
	if m_tHotfixCallBack[strOldFile] ~= nil then
		for _, hotfixcb in ipairs(m_tHotfixCallBack[strOldFile]) do
			hotfixcb(G6HotFix.HotFixCount, strOldFile, G6HotFix.m_Loaded[strOldFile])
		end
	end

	-- Hotfix执行完毕
	for _, callback in ipairs(m_tPostHotfixCallback) do
		callback({strOldFile})
	end
end

--- 使用NewFile对OldFile进行HotFix。
--- 一次性读取所有文件module信息后，同时做Patch，避免文件依赖问题
---@param listOldFile table 旧文件列表
---@param listNewFile table 新文件列表
local print = G6HotFix.print
function G6HotFix.HotFixFile(listOldFile, listNewFile)
	if listNewFile == nil then
		listNewFile = listOldFile
	end

	if listOldFile == nil or #listOldFile == 0 or #listOldFile ~= #listNewFile then
		print("nothing to hotfix")
		return
	end

	-- 有Hotfix，触发PreHotfix
	for _, callback in ipairs(m_tPreHotfixCallback) do
		callback(listOldFile)
	end

	local tmploaded = {}
	local tmploadedmod = {}
	for k, v in pairs(G6HotFix.m_Loaded) do
		if listOldFile[k] ~= nil then
			table.insert(tmploaded, k)
			table.insert(tmploadedmod, v)
		end
	end

	SandBox.Init(tmploaded, tmploadedmod)

	local listOldModule = {}
	local listNewModule = {}
	local listInitModule = {}
	local listFileEnv = {}

	for i,strOldFile in ipairs(listOldFile) do		
		local strNewFile = listNewFile[i]

		if G6HotFix.m_Loaded[strOldFile] == nil then
			print("file not loaded : ", strOldFile, "load file for new : ", strNewFile, G6HotFix.m_Loaded, TableToString(G6HotFix.m_Loaded, 62))
			G6HotFix.RequireFile(strNewFile)
		else
			local _f, err, _fileenv = SelfLoadFile(strNewFile)
			if _f ~= nil then
				strexmsg = strNewFile
				local succ, _newModlue = xpcall(_f, error_handler)
				if succ == false then
					SandBox.Clear()
					return
				end
		    	if _newModlue then
		    		for k, v in pairs(_fileenv) do
		      			_newModlue[k] = v
		    		end
		        else
		  	        _newModlue = _fileenv
		    	end
		    	listOldModule[#listOldModule+1] = G6HotFix.m_Loaded[strOldFile]
		    	listNewModule[#listNewModule+1] = _newModlue
				listFileEnv[#listFileEnv+1] = _fileenv
			else
				SandBox.Clear()
				return
			end
		end
	end

	G6HotFix.UpdateModule(listOldModule, listNewModule, listFileEnv)

	SandBox.Clear()

	G6HotFix.HotFixCount = G6HotFix.HotFixCount + 1

	for i,strOldFile in ipairs(listOldFile) do
		if G6HotFix.UseNewModuleWhenHotfix then
			--change to new or will be nil
			G6HotFix.m_Loaded[strOldFile] = listNewModule[i]
			package.loaded[strOldFile] = listNewModule[i]
		end

		for _, callback in ipairs(m_tFileLoadCallBack) do
			callback(G6HotFix.m_Loaded[strOldFile], strOldFile, true)
		end
		if m_tHotfixCallBack[strOldFile] ~= nil then
			for _, hotfixcb in ipairs(m_tHotfixCallBack[strOldFile]) do
				hotfixcb(G6HotFix.HotFixCount, strOldFile, G6HotFix.m_Loaded[strOldFile])
			end
		end
	end
	
	-- Hotfix执行完毕
	for _, callback in ipairs(m_tPostHotfixCallback) do
		callback(listOldFile)
	end
end

--- 重新加载指定名称的lua文件
---@param filename string @文件名称
function G6HotFix.ReloadFile(filename)
	G6HotFix.ClearLoadedModuleByName(filename)
	return G6HotFix.RequireFile(filename)
end

--- 加入hotfix白名单机制，有些文件不能hotfix
---@param whitelist table require的文件名集合
function G6HotFix.AddWhiteList(whitelist)
	G6HotFix.WhiteList = whitelist
end

require = G6HotFix.RequireFile

return G6HotFix
