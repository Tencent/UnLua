local M = UnLua.Class()

function M:ReceiveBeginPlay()
	local BehaviorTree = UE.UObject.Load("/Game/Core/Blueprints/AI/BT_Enemy")
	self:RunBehaviorTree(BehaviorTree)
end

return M
