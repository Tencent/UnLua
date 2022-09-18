local M = {}

local function GetBooleanArg(Args, Name, DefaultValue)
    if Args[Name] == nil then
        return DefaultValue
    end
    return not not Args[Name]
end

local function MakeLuaFunction(Module, Prefix, Handler, No)
    local Name = string.format("%s_%d", Prefix, No)
    if not Module[Name] then
        Module[Name] = Handler
        return Name
    end
    
    return MakeLuaFunction(Module, Prefix, Handler, No + 1)
end

function M.BindKey(Module, KeyName, KeyEvent, Handler, Args)
    Args = Args or {}
    Module.__UnLuaInputBindings = Module.__UnLuaInputBindings or {}
    local FunctionName = MakeLuaFunction(Module, string.format("UnLuaInput_%s_%s", KeyName, KeyEvent), Handler, 0)
    local Bindings = Module.__UnLuaInputBindings
    table.insert(Bindings, function(Manager, Class)
        local BindingObject = Manager:GetOrAddBindingObject(Class, UE.UInputKeyDelegateBinding)
        for _, OldBinding in pairs(BindingObject.InputKeyDelegateBindings) do
            if OldBinding.FunctionNameToBind == FunctionName then
                -- PIE下这个蓝图可能已经绑定过了
                Manager:Override(Class, "InputAction", FunctionName)
                return
            end
        end

        local InputChord = UE.FInputChord()
        InputChord.Key = UE.EKeys[KeyName]
        InputChord.bShift = not not Args.Shift
        InputChord.bCtrl = not not Args.Ctrl
        InputChord.bAlt = not not Args.Alt
        InputChord.bCmd = not not Args.Cmd

        local Binding = UE.FBlueprintInputKeyDelegateBinding()
        Binding.bConsumeInput = GetBooleanArg(Args, "ConsumeInput", true)
        Binding.bExecuteWhenPaused = GetBooleanArg(Args, "ExecuteWhenPaused", false)
        Binding.bOverrideParentBinding = GetBooleanArg(Args, "OverrideParentBinding", true)
        Binding.InputChord = InputChord
        Binding.InputKeyEvent = UE.EInputEvent["IE_" .. KeyEvent]
        Binding.FunctionNameToBind = FunctionName
        BindingObject.InputKeyDelegateBindings:Add(Binding)
        
        Manager:Override(Class, "InputAction", FunctionName)
    end)
end

function M.PerformBindings(Module, Manager, Class)
    local Bindings = Module.__UnLuaInputBindings
    if not Bindings then
        return
    end

    for _, Binding in ipairs(Bindings) do
        xpcall(Binding, function(Error)
            UnLua.LogError(Error .. "\n" .. debug.traceback())
        end, Manager, Class)
    end
end

return M