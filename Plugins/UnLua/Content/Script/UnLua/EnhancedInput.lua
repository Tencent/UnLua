local Input = UnLua.Input

local M = {}

local function MakeBinding(BindingClass, Args)
    local Ret = BindingClass()
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

local SignatureFunctionNames = {
    "EnhancedInputActionDigital",
    "EnhancedInputActionAxis1D",
    "EnhancedInputActionAxis2D",
    "EnhancedInputActionAxis3D"
}

--- 为当前模块绑定操作输入响应
---@param Module table @需要绑定的lua模块
---@param InputAction string @绑定的操作资源路径，是个UInputAction
---@param TriggerEvent string @绑定的事件名称，参考ETriggerEvent下的命名
---@param Handler fun(Key:FKey) @事件响应回调函数
---@param Args table @[opt]扩展参数
function M.BindAction(Module, ActionPath, TriggerEvent, Handler, Args)
    Args = Args or {}
    Module.__UnLuaInputBindings = Module.__UnLuaInputBindings or {}
    local InputAction = UE.UObject.Load(ActionPath)
    if not InputAction then
        return
    end

    local SignatureFunctionName = SignatureFunctionNames[InputAction.ValueType + 1]
    local FunctionName = MakeLuaFunction(Module, string.format("UnLuaInput_%s_%s", InputAction:GetName(), TriggerEvent), Handler, 0)
    local Bindings = Module.__UnLuaInputBindings
    table.insert(Bindings, function(Manager, Class)
        local BindingObject = Manager:GetOrAddBindingObject(Class, UE.UEnhancedInputActionDelegateBinding)
        for _, OldBinding in pairs(BindingObject.InputActionDelegateBindings) do
            if OldBinding.FunctionNameToBind == FunctionName then
                -- PIE下这个蓝图可能已经绑定过了
                Manager:Override(Class, SignatureFunctionName, FunctionName)
                return
            end
        end

        local Binding = MakeBinding(UE.FBlueprintEnhancedInputActionBinding, Args)
        Binding.InputAction = InputAction
        Binding.TriggerEvent = UE.ETriggerEvent[TriggerEvent]
        Binding.FunctionNameToBind = FunctionName
        BindingObject.InputActionDelegateBindings:Add(Binding)

        Manager:Override(Class, SignatureFunctionName, FunctionName)
    end)
end

return M