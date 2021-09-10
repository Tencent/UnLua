------------------------------------------
--- G6HotFix 的加载，用于加载整个Lua系统
------------------------------------------

-- 
-- G6采用Require来加载Lua文件，以提供HotFix支持，
-- 所以在加载其他Lua文件之前，要先将HotFix加载
-- 

-- Load HotFix
print("Load HotFix...");

local hotfix_func,err = UELoadLuaFile("G6Hotfix.lua");
if (hotfix_func) and type(hotfix_func) == "function" then
	package.loaded["g6hotfix"] = hotfix_func();
	_G.G6HotFix = package.loaded["g6hotfix"];
else
	print("Require HotFix Fail " .. tostring(err));
end

-- _G.G6HotFix = require "G6Core.Misc.G6Hotfix"
print("Load HotFix --->");
--
-- 所有之后的Lua文件使用的是hotfix的require了
--

--- 重新加载某个文件
---@param filepath string
function ReloadFile(filepath)
	if nil == filepath then
		return
	end

	_G.G6HotFix.ReloadFile(filepath)
end
 
--- 重新加载所有文件
function ReloadAll()
	_G.G6HotFix.ClearLoadedModule()

end

--- 打印表内容
---@overload fun(tbl: table):string
---@param TableToPrint table
---@param MaxIntent number
---@return string
function _G.TableToString (TableToPrint, MaxIntent)
    local HandlerdTable = {}
    local function ItretePrintTable(TP, Indent)
        if not Indent then Indent = 0 end
        if type(TP) ~= "table" then return tostring(TP) end

        if(Indent > MaxIntent) then return tostring(TP) end

        if HandlerdTable[TP] then
            return "";
        end
        HandlerdTable[TP] = true
        local StrToPrint = string.rep(" ", Indent) .. "{\r\n"
        Indent = Indent + 2 
        for k, v in pairs(TP) do
            StrToPrint = StrToPrint .. string.rep(" ", Indent)
            if (type(k) == "number") then
                StrToPrint = StrToPrint .. "[" .. k .. "] = "
            elseif (type(k) == "string") then
                StrToPrint = StrToPrint  .. k ..  "= "   
            else
                StrToPrint = StrToPrint  .. tostring(k) ..  " = "   
            end
            if (type(v) == "number") then
                StrToPrint = StrToPrint .. v .. ",\r\n"
            elseif (type(v) == "string") then
                StrToPrint = StrToPrint .. "\"" .. v .. "\",\r\n"
            elseif (type(v) == "table") then
                StrToPrint = StrToPrint .. tostring(v) .. ItretePrintTable(v, Indent + 2) .. ",\r\n"
            else
                StrToPrint = StrToPrint .. "\"" .. tostring(v) .. "\",\r\n"
            end
        end
        StrToPrint = StrToPrint .. string.rep(" ", Indent-2) .. "}"
        return StrToPrint
    end

    if MaxIntent == nil then
        MaxIntent = 64
    end
    return ItretePrintTable(TableToPrint)
end

--- hotfix所有修改的文件
--- @param bNotPrintHotfixFile boolean @是否需要打印日志
function HotFix(bNotPrintHotfixFile)
	_G.G6HotFix.HotFixModifyFile(bNotPrintHotfixFile)
end

ReloadAll()

print("PRINT G6ENV : ", TableToString(G6ENV) )