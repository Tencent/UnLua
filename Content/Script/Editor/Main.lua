UnLua.ConsoleCommand.Add("unlua.test", "this is a test command", function(Args)
    print("unlua.test executed Args=", table.unpack(Args))
end)

-- UnLua.ConsoleCommand.Remove("unlua.test")

-- 解除以下注释，会在编辑器启动时扩展对应的菜单

-- UnLua.Editor.ExtendMenu("ContentBrowser.AssetContextMenu", {
--     {
--         Name = "Parent",
--         Label = "扩展子菜单",
--         Section = "UnLuaTutorial",
--         Asset = "/Game/Editor/Menus/AssetContext/EUBP_Parent",
--         Children = {
--             {
--                 Name = "Child1",
--                 Label = "子菜单选项1",
--                 Asset = "/Game/Editor/Menus/AssetContext/EUBP_Child1"
--             },
--             {
--                 Name = "Child2",
--                 Label = "子菜单选项2",
--                 Asset = "/Game/Editor/Menus/AssetContext/EUBP_Child2"
--             }
--         }
--     }
-- })

-- UnLua.Editor.ExtendMenu("LevelEditor.LevelEditorToolBar", {
--     {
--         Name = "ToolBarExtension",
--         Label = "扩展工具栏按钮",
--         Section = "UnLuaTutorial",
--         Asset = "/Game/Editor/Menus/LevelEditorToolBar/EUBP_ToolBarButton"
--     },
-- })

-- UnLua.Editor.ExtendMenu("MainFrame.MainMenu", {
--     {
--         Name = "Parent",
--         Label = "扩展子菜单",
--         Section = "UnLuaTutorial",
--         Asset = "/Game/Editor/Menus/MainMenu/EUBP_Parent",
--         Children = {
--             {
--                 Name = "Child1",
--                 Label = "子菜单选项1",
--                 Asset = "/Game/Editor/Menus/MainMenu/EUBP_Child1"
--             },
--             {
--                 Name = "Child2",
--                 Label = "子菜单选项2",
--                 Asset = "/Game/Editor/Menus/MainMenu/EUBP_Child2"
--             }
--         }
--     },
-- })