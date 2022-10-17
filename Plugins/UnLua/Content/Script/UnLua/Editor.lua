local M = {}

local function GetClassPath(AssetPath)
    local LastPart = AssetPath:sub(AssetPath:find("/[^/]*$") + 1)
    return AssetPath .. "." .. LastPart .. "_C"
end

--- 扩展UE编辑器菜单
---@param string MenuName
---@param table Args
function M.ExtendMenu(MenuName, Args)
    for _, Arg in ipairs(Args) do
        assert(type(Arg.Name) == "string")
        if Arg.Asset then
            local ClassPath = GetClassPath(Arg.Asset)
            local Class = UE.UClass.Load(ClassPath)
            local Entry = NewObject(Class)
            local Data = Entry.Data
            Data.Menu = MenuName
            Data.Name = Arg.Name
            Data.Section = Arg.Section or Data.Section
            Data.Label = Arg.Label or Data.Label
            Data.ToolTip = Arg.ToolTip or Data.ToolTip

            if Arg.Icon then
                Data.Icon.StyleSetName = Arg.Icon.StyleSetName
                Data.Icon.StyleName = Arg.Icon.StyleName
                Data.Icon.SmallStyleName = Arg.Icon.SmallStyleName
            end

            if Arg.SubMenu then
                Data.Advanced.bIsSubMenu = true
            end

            UE.UToolMenus.AddMenuEntryObject(Entry)
        end

        if Arg.Children then
            M.ExtendMenu(MenuName .. "." .. Arg.Name, Arg.Children)
        end
    end
end

return M