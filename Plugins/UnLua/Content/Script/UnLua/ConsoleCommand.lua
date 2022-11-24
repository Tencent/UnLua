local M = {}

---增加一个控制台命令
---@param Name string@名称
---@param Help string@控制台命令的说明
---@param Execute function@当命令被执行时的回调
function M.Add(Name, Help, Execute)
    if type(Name) == "table" then
        Name = Name.Name
        Help = Name.Help
        Execute = Name.Execute
    end

    local Delegate = {
        UE.UUnLuaFunctionLibrary,
        function(_, Args)
            Execute(Args:ToTable())
        end
    }
    UE.UUnLuaFunctionLibrary.AddConsoleCommand(Name, Help, Delegate)
end

---移除指定的控制台命令
---@param Name string@名称
function M.Remove(Name)
    UE.UUnLuaFunctionLibrary.RemoveConsoleCommand(Name)
end

return M