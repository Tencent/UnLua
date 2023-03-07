local M =  UnLua.Class()

function M:Construct()
    self.TestText.OnTextChanged:Add(self, function()
        self.TestText:SetText("LuaLua")
    end)
end

return M