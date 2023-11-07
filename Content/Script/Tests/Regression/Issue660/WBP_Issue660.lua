local M =  UnLua.Class()

function M:Construct()
    local ok, err = pcall(function()
        self.Button_Test1.OnClicked:Add(self, self.Callback)
        self.Button_Test2.OnClicked:Add(self, self.Callback)
        self.EditableText_Test.OnTextCommitted:Add(self, self.Callback)
    end)
    assert(string.find(err, "delegate handler signatures are not compatible"))
end

function M:Callback(A, B)
    print("Callback -> ", self, A, B)
end

return M