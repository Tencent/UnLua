local M = UnLua.Class()

function M:Construct()
    self.Counter = 0
    for i = 1, 3 do
        local Entry = self.DynamicEntryBox_List:BP_CreateEntry()
        Entry:InitState(self, ("Result" .. i))
    end
end

function M:Tick()
    self.Counter = self.Counter + 1
    self.TestDelegate:Broadcast(self.Counter)
end

return M
