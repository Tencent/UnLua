require "UnLua"

local BP_AIController_C = Class()

function BP_AIController_C:ReceiveBeginPlay()
	local BehaviorTree = UE.UObject.Load("/Game/Core/Blueprints/AI/BT_Enemy")
	self:RunBehaviorTree(BehaviorTree)
end

return BP_AIController_C
