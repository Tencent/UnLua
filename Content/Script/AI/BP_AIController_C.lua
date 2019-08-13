require "UnLua"

local BP_AIController_C = Class()

--BP_AIController_C.BehaviorTree = UObject.Load("/Game/Core/Blueprints/AI/BT_Enemy")
BP_AIController_C.BehaviorTree = LoadObject("/Game/Core/Blueprints/AI/BT_Enemy")

function BP_AIController_C:ReceiveBeginPlay()
	--local BehaviorTree = UObject.Load("/Game/Core/Blueprints/AI/BT_Enemy")
	self:RunBehaviorTree(self.BehaviorTree)
end

return BP_AIController_C
