local M = {}

local function GetBooleanArg(Args, Name, DefaultValue)
    if Args[Name] == nil then
        return DefaultValue
    end
    return not not Args[Name]
end

local function MakeBinding(BindingClass, Args)
    local Ret = BindingClass()
    Ret.bConsumeInput = GetBooleanArg(Args, "ConsumeInput", true)
    Ret.bExecuteWhenPaused = GetBooleanArg(Args, "ExecuteWhenPaused", false)
    Ret.bOverrideParentBinding = GetBooleanArg(Args, "OverrideParentBinding", true)
    return Ret
end

local function MakeLuaFunction(Module, Prefix, Handler, No)
    local Name = string.format("%s_%d", Prefix, No)
    if not Module[Name] then
        Module[Name] = Handler
        return Name
    end
    
    return MakeLuaFunction(Module, Prefix, Handler, No + 1)
end

local Modifiers = { "Ctrl", "Alt", "Shift", "Cmd" }

local function GetModifierSuffix(Args)
    local Array = { "" }
    for _, Modifier in ipairs(Modifiers) do
        if Args[Modifier] then
            table.insert(Array, Modifier)
        end
    end
    return table.concat(Array, "_")
end

--- 为当前模块绑定按键输入响应
---@param Module table @需要绑定的lua模块
---@param KeyName string @绑定的按键名称，参考EKeys下的命名
---@param KeyEvent string @绑定的事件名称，参考EInputEvent下的命名，不需要 “IE_” 前缀
---@param Handler fun(Key:FKey) @事件响应回调函数
---@param Args table @[opt]扩展参数 使用 Ctrl/Shift/Alt/Cmd = true 来控制组合键
function M.BindKey(Module, KeyName, KeyEvent, Handler, Args)
    Args = Args or {}
    Module.__UnLuaInputBindings = Module.__UnLuaInputBindings or {}
    local ModifierSuffix = GetModifierSuffix(Args)
    local FunctionName = MakeLuaFunction(Module, string.format("UnLuaInput_%s_%s%s", KeyName, KeyEvent, ModifierSuffix), Handler, 0)
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

        local Binding = MakeBinding(UE.FBlueprintInputKeyDelegateBinding, Args)
        Binding.InputChord = InputChord
        Binding.InputKeyEvent = UE.EInputEvent["IE_" .. KeyEvent]
        Binding.FunctionNameToBind = FunctionName
        BindingObject.InputKeyDelegateBindings:Add(Binding)
        
        Manager:Override(Class, "InputAction", FunctionName)
    end)
end

--- 为当前模块绑定操作输入响应
---@param Module table @需要绑定的lua模块
---@param ActionName string @绑定的操作名称，对应 “项目设置->输入” 中设置的命名
---@param KeyEvent string @绑定的事件名称，参考EInputEvent下的命名，不需要 “IE_” 前缀
---@param Handler fun(Key:FKey) @事件响应回调函数
---@param Args table @[opt]扩展参数
function M.BindAction(Module, ActionName, KeyEvent, Handler, Args)
    Args = Args or {}
    Module.__UnLuaInputBindings = Module.__UnLuaInputBindings or {}
    local FunctionName = MakeLuaFunction(Module, string.format("UnLuaInput_%s_%s", ActionName, KeyEvent), Handler, 0)
    local Bindings = Module.__UnLuaInputBindings
    table.insert(Bindings, function(Manager, Class)
        local BindingObject = Manager:GetOrAddBindingObject(Class, UE.UInputActionDelegateBinding)
        for _, OldBinding in pairs(BindingObject.InputActionDelegateBindings) do
            if OldBinding.FunctionNameToBind == FunctionName then
                -- PIE下这个蓝图可能已经绑定过了
                Manager:Override(Class, "InputAction", FunctionName)
                return
            end
        end

        local Binding = MakeBinding(UE.FBlueprintInputActionDelegateBinding, Args)
        Binding.InputActionName = ActionName
        Binding.InputKeyEvent = UE.EInputEvent["IE_" .. KeyEvent]
        Binding.FunctionNameToBind = FunctionName
        BindingObject.InputActionDelegateBindings:Add(Binding)
        
        Manager:Override(Class, "InputAction", FunctionName)
    end)
end

--- 为当前模块绑定轴输入响应
---@param Module table @需要绑定的lua模块
---@param AxisName string @绑定的轴名称，对应 “项目设置->输入” 中设置的命名
---@param Handler fun(AxisValue:number) @事件响应回调函数
---@param Args table @[opt]扩展参数
function M.BindAxis(Module, AxisName, Handler, Args)
    Args = Args or {}
    Module.__UnLuaInputBindings = Module.__UnLuaInputBindings or {}
    local FunctionName = MakeLuaFunction(Module, string.format("UnLuaInput_%s", AxisName), Handler, 0)
    local Bindings = Module.__UnLuaInputBindings
    table.insert(Bindings, function(Manager, Class)
        local BindingObject = Manager:GetOrAddBindingObject(Class, UE.UInputAxisDelegateBinding)
        for _, OldBinding in pairs(BindingObject.InputAxisDelegateBindings) do
            if OldBinding.FunctionNameToBind == FunctionName then
                -- PIE下这个蓝图可能已经绑定过了
                Manager:Override(Class, "InputAxis", FunctionName)
                return
            end
        end

        local Binding = MakeBinding(UE.FBlueprintInputAxisDelegateBinding, Args)
        Binding.InputAxisName = AxisName
        Binding.FunctionNameToBind = FunctionName
        BindingObject.InputAxisDelegateBindings:Add(Binding)
        
        Manager:Override(Class, "InputAxis", FunctionName)
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