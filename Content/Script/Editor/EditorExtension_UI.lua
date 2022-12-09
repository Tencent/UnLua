---@type WBP_Intro_C
local M = UnLua.Class()

function M:Construct()
    self.Button_ActiveAll.OnClicked:Add(self, self.OnActiveAll)
    self.Button_DeactiveAll.OnClicked:Add(self, self.OnDeactiveAll)

    self.CheckBox_MainMenu.OnCheckStateChanged:Add(self, self.OnMainMenu)
    self.CheckBox_AssetContextMenu.OnCheckStateChanged:Add(self, self.OnAssetContextMenu)
    self.CheckBox_Toolbar.OnCheckStateChanged:Add(self, self.OnToolbar)

    self.CheckBoxes = {
        self.CheckBox_MainMenu, 
        self.CheckBox_AssetContextMenu,
        self.CheckBox_Toolbar
    }
end

function M:OnActiveAll()
    for _, CheckBox in ipairs(self.CheckBoxes) do
        CheckBox:SetIsChecked(true)
        CheckBox.OnCheckStateChanged:Broadcast(true)
    end
end

function M:OnDeactiveAll()
    for _, CheckBox in ipairs(self.CheckBoxes) do
        CheckBox:SetIsChecked(false)
        CheckBox.OnCheckStateChanged:Broadcast(false)
    end
end

local function ResetMenu(Menu)
    local Menus = UE.UToolMenus.Get()
    Menus:RemoveSection(Menu, "UnLuaTutorial")
    Menus:RefreshAllWidgets()
end

function M:OnMainMenu(bIsChecked)
    if not bIsChecked then
        return ResetMenu("MainFrame.MainMenu")
    end

    UnLua.Editor.ExtendMenu("MainFrame.MainMenu", {
        {
            Name = "Parent",
            Label = "扩展子菜单",
            Section = "UnLuaTutorial",
            Asset = "/Game/Editor/Menus/MainMenu/EUBP_Parent",
            Children = {
                {
                    Name = "Child1",
                    Label = "子菜单选项1",
                    Asset = "/Game/Editor/Menus/MainMenu/EUBP_Child1"
                },
                {
                    Name = "Child2",
                    Label = "子菜单选项2",
                    Asset = "/Game/Editor/Menus/MainMenu/EUBP_Child2"
                }
            }
        },
    })
end

function M:OnAssetContextMenu(bIsChecked)
    if not bIsChecked then
        return ResetMenu("ContentBrowser.AssetContextMenu")
    end

    UnLua.Editor.ExtendMenu("ContentBrowser.AssetContextMenu", {
        {
            Name = "Parent",
            Label = "扩展子菜单",
            Section = "UnLuaTutorial",
            Asset = "/Game/Editor/Menus/AssetContext/EUBP_Parent",
            Children = {
                {
                    Name = "Child1",
                    Label = "子菜单选项1",
                    Asset = "/Game/Editor/Menus/AssetContext/EUBP_Child1"
                },
                {
                    Name = "Child2",
                    Label = "子菜单选项2",
                    Asset = "/Game/Editor/Menus/AssetContext/EUBP_Child2"
                }
            }
        }
    })
end

function M:OnToolbar(bIsChecked)
    if not bIsChecked then
        return ResetMenu("LevelEditor.LevelEditorToolBar")
    end

    UnLua.Editor.ExtendMenu("LevelEditor.LevelEditorToolBar", {
        {
            Name = "ToolBarExtension",
            Label = "扩展工具栏按钮",
            Section = "UnLuaTutorial",
            Asset = "/Game/Editor/Menus/LevelEditorToolBar/EUBP_ToolBarButton"
        },
    })
end

function M:Destruct()
    self:OnDeactiveAll()
end

return M
