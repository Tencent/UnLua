require "UnLua"

local BP_Game_C = Class()

--function BP_Game_C:UserConstructionScript()
--end

function BP_Game_C:ReceiveBeginPlay()
	self.EnemySpawnInterval = 2.0
	self.MaxEnemies = 4
	self.AliveEnemies = 0
	self.SpawnOrigin = UE4.FVector(650.0, 0.0, 100.0)
	self.SpawnLocation = UE4.FVector()
	self.AICharacterClass = UE4.UClass.Load("/Game/Core/Blueprints/AI/BP_AICharacter.BP_AICharacter_C")
	UE4.UKismetSystemLibrary.K2_SetTimerDelegate({self, BP_Game_C.SpawnEnemy}, self.EnemySpawnInterval, true)
	UE4.UKismetSystemLibrary.K2_SetTimerDelegate({self, BP_Game_C.MainTick}, 1.0, true)
end

function BP_Game_C:SpawnEnemy()
	local PlayerCharacter = UE4.UGameplayStatics.GetPlayerCharacter(self, 0)
	if self.AliveEnemies < self.MaxEnemies and PlayerCharacter then
		UE4.UNavigationSystemV1.K2_GetRandomReachablePointInRadius(self, self.SpawnOrigin, self.SpawnLocation, 2000)		--
		self.SpawnLocation.Z = self.SpawnLocation.Z + 100
		local Target = PlayerCharacter:K2_GetActorLocation()
		local SpawnRotation = UE4.UKismetMathLibrary.FindLookAtRotation(self.SpawnLocation, Target)
		UE4.UAIBlueprintHelperLibrary.SpawnAIFromClass(self, self.AICharacterClass, nil, self.SpawnLocation, SpawnRotation)		--
		self.AliveEnemies = self.AliveEnemies + 1
		if self.AliveEnemies > self.MaxEnemies then
			self.AliveEnemies = self.MaxEnemies
		end
	end
end


function BP_Game_C:MainTick()
	_G.HotFix(true)
end

function BP_Game_C:NotifyEnemyDied()
	self.AliveEnemies = self.AliveEnemies - 1
	if self.AliveEnemies < 0 then
		self.AliveEnemies = 0
	end
end

return BP_Game_C
