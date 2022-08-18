--[[
    说明：可以通过对委托原生接口的调用，完成对UI事件的监听

    小提示：
    绑定/解绑成对出现，是编写代码的良好习惯
]]--

local FLinearColor = UE.FLinearColor

local M = UnLua.Class()

function M:Construct()
    self.ClickMeButton.OnClicked:Add(self, self.OnButtonClicked)
    self.ClickMeCheckBox.OnCheckStateChanged:Add(self, self.OnCheckBoxToggled)

    -- 相当于在蓝图中的 Set Timer by Event
    self.TimerHandle = UE.UKismetSystemLibrary.K2_SetTimerDelegate({ self, self.OnTimer }, 1, true)
end

function M:OnButtonClicked()
    local r = math.random()
    local g = math.random()
    local b = math.random()

    self.ClickMeButton:SetBackgroundColor(FLinearColor(r, g, b, 1))
end

function M:OnCheckBoxToggled(on)
    if on then
        self.CheckBoxText:SetText("已选中")
    else
        self.CheckBoxText:SetText("未选中")
    end
end

function M:OnTimer()
    local seconds = UE.UKismetSystemLibrary.GetGameTimeInSeconds(self)
    self.GameTimeTextBlock:SetText(string.format("游戏时长：%d 秒", math.floor(seconds)))
end

function M:Destruct()
    -- 在UMG被销毁时尽量清理已绑定的委托，不清理则会在切换Map时自动清理
    self.ClickMeButton.OnClicked:Remove(self, self.OnButtonClicked)
    self.ClickMeCheckBox.OnCheckStateChanged:Remove(self, self.OnCheckBoxToggled)

    -- 相当于在蓝图中的 Clear and Invalidate Timer by Handle
    UE.UKismetSystemLibrary.K2_ClearAndInvalidateTimerHandle(self, self.TimerHandle)
end

return M
