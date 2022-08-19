--[[
    说明：创建原生容器时通常需要指定参数类型，来确定容器内存放的数据类型
    
    例如：
    local array = TArray({ElementType})
    local set = TSet({ElementType})
    local map = TMap({KeyType}, {ValueType})

    参数类型        示例             实际类型
    boolean        true             Boolean
    number         0                Interger
    string         ""               String
    table          FVector          Vector
    userdata       FVector(1,1,1)   Vector

    创建完成后，和原来的原生容器类型使用方式是相同的，更多接口可以参考源码：
    TArray      Plugins\UnLua\Source\UnLua\Private\BaseLib\LuaLib_Array.cpp
    TSet        Plugins\UnLua\Source\UnLua\Private\BaseLib\LuaLib_Set.cpp
    TMap        Plugins\UnLua\Source\UnLua\Private\BaseLib\LuaLib_Map.cpp
]] --

local Screen = require "Tutorials.Screen"

local M = UnLua.Class()

local function print_intro()
    local msg =
        [[
使用以下按键执行各个示例，并在控制台查看输出：

数字1：TArray
数字2：TSet
数字3：TMap

—— 本示例来自 "Content/Script/Tutorials.06_NativeContainers.lua"
]] ..
        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    Screen.Print(msg)
end

function M:ReceiveBeginPlay()
    print_intro()
end

local function dump_array(array)
    local ret = {}
    for i = 1, array:Length() do
        table.insert(ret, array:Get(i))
    end
    return "[" .. table.concat(ret, ",") .. "]"
end

function M:One_Pressed()
    print_intro()
    Screen.Print("TArray")

    print("========== TArray ==========")
    print("注：索引从1开始")

    local array = UE.TArray(0)
    print("New:          ", dump_array(array))

    array:Add(1)
    array:Add(2)
    array:Add(3)
    array:Add(3)
    print("Add:          ", dump_array(array))

    local index = array:AddUnique(1)
    print("AddUnique(1): ", dump_array(array), "Returns:", index)

    array:Remove(1)
    print("Remove(1):    ", dump_array(array))

    array:RemoveItem(3)
    print("RemoveItem(3):", dump_array(array))

    array:Insert(4, 1)
    print("Insert(1, 4): ", dump_array(array))

    array:Shuffle()
    print("Shuffle():    ", dump_array(array))

    array:Clear()
    print("Clear:        ", dump_array(array))
end

local function dump_set(set)
    local array = set:ToArray()
    local ret = {}
    for i = 1, array:Length() do
        table.insert(ret, array:Get(i))
    end
    return "(" .. table.concat(ret, ",") .. ")"
end

function M:Two_Pressed()
    print_intro()
    Screen.Print("TSet")

    print("========== TSet ==========")
    local set = UE.TSet(0)
    print("New:          ", dump_set(set))

    set:Add(1)
    set:Add(1)
    set:Add(2)
    set:Add(2)
    set:Add(3)
    set:Add(3)
    print("Add:          ", dump_set(set))
    print("Length:       ", set:Length())
    print("Contains(3):  ", set:Contains(3))

    set:Clear()
    print("Clear:        ", dump_set(set))
end

local function dump_map(map)
    local ret = {}
    local keys = map:Keys()
    for i = 1, keys:Length() do
        local key = keys:Get(i)
        local value = map:Find(key)
        table.insert(ret, key .. ":" .. tostring(value))
    end
    return "{" .. table.concat(ret, ",") .. "}"
end

function M:Three_Pressed()
    print_intro()
    Screen.Print("TMap")

    print("========== TMap ==========")
    local map = UE.TMap(0, true)
    print("New:          ", dump_map(map))

    map:Add(1, true)
    map:Add(2, false)
    map:Add(3, true)
    print("Add:          ", dump_map(map))

    map:Remove(2)
    print("Remove(2):    ", dump_map(map))

    local value = map:Find(3)
    print("Find(3):      ", dump_map(map), "Returns:", value)

    map:Clear()
    print("Clear:        ", dump_map(map))
end

return M
