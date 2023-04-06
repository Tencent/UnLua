---@type BP_UnLuaTestStubActor_C
local M = UnLua.Class()

function M:Initialize()
    self.InitializeCalled = true
end

function M:ReceiveBeginPlay()
    self.ReceiveBeginPlayCalled = true
    self.ClassReferenceProperty = self:GetClass()
    self.SoftClassReferenceProperty:Set(self:GetClass())
    self.ObjectReferenceProperty = self
end

function M:ReceiveTick()
    self.TickCalled = true
    self:K2_DestroyActor()
end

function M:ReceiveDestroyed()
    self.ReceiveDestroyedCalled = true
end

function M:Greeting(txt)
    return self.Overridden.Greeting(self, txt) .. " Lua"
end

function M:RunTest()
    local errors = {}

    local test = function(key, expected, getter)
        local actual = self[key]
        if getter then
            actual = getter(actual)
        end
        if actual ~= expected then
            table.insert(errors, string.format("self.%s should be %s, but got %s", key, expected, actual))
        end
    end

    local ok, err = pcall(function()
        test("InitializeCalled", true)
        test("ReceiveBeginPlayCalled", true)
        test("TickCalled", true)
        test("ReceiveDestroyedCalled", true)

        test("BooleanProperty", true)
        test("ByteProperty", 1)
        test("IntegerProperty", 2)
        test("Integer64Property", 3)
        test("FloatProperty", 4.5)
        test("FNameProperty", "TestName")
        test("FStringProperty", "TestString")
        test("FTextProperty", "TestText", function(v) return tostring(v) end)
        test("FVectorProperty", UE.FVector(1,2,3))
        test("FRotatorProperty", UE.FRotator(2,3,1))

        test("FTransformProperty", UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(1,2,3)))
        test("ObjectReferenceProperty", self)
        test("ClassReferenceProperty", self:GetClass())
        test("SoftObjectReferenceProperty", "SkySphere", function(v) return v:LoadSynchronous():GetName() end)
        test("SoftClassReferenceProperty", self:GetClass(), function(v) return v:Get() end)
        test("EnumProperty", UE.ECollisionChannel.Pawn)
    end)

    if err then
        return err
    end

    if #errors > 0 then
        return table.concat(errors, "\n")
    end

    return ""
end

return M
