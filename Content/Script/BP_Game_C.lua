---@type BP_Game_C
local M = UnLua.Class()

function M:ReceiveBeginPlay()
	self.EnemySpawnInterval = 2.0
	self.MaxEnemies = 4
	self.AliveEnemies = 0
	self.SpawnOrigin = UE.FVector(650.0, 0.0, 100.0)
	self.SpawnLocation = UE.FVector()
	self.AICharacterClass = UE.UClass.Load("/Game/Core/Blueprints/AI/BP_AICharacter.BP_AICharacter_C")
	UE.UKismetSystemLibrary.K2_SetTimerDelegate({self, M.SpawnEnemy}, self.EnemySpawnInterval, true)
end

function M:SpawnEnemy()
	local PlayerCharacter = UE.UGameplayStatics.GetPlayerCharacter(self, 0)
	if self.AliveEnemies < self.MaxEnemies and PlayerCharacter then
		UE.UNavigationSystemV1.K2_GetRandomReachablePointInRadius(self, self.SpawnOrigin, self.SpawnLocation, 2000)
		self.SpawnLocation.Z = self.SpawnLocation.Z + 100
		local Target = PlayerCharacter:K2_GetActorLocation()
		local SpawnRotation = UE.UKismetMathLibrary.FindLookAtRotation(self.SpawnLocation, Target)
		UE.UAIBlueprintHelperLibrary.SpawnAIFromClass(self, self.AICharacterClass, nil, self.SpawnLocation, SpawnRotation)
		self.AliveEnemies = self.AliveEnemies + 1
		if self.AliveEnemies > self.MaxEnemies then
			self.AliveEnemies = self.MaxEnemies
		end
	end
end

function M:NotifyEnemyDied()
	self.AliveEnemies = self.AliveEnemies - 1
	if self.AliveEnemies < 0 then
		self.AliveEnemies = 0
	end
end

return M
