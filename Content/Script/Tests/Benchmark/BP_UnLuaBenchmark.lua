local M = UnLua.Class()

local Start = UE.UUnLuaBenchmarkFunctionLibrary.Start
local Stop = UE.UUnLuaBenchmarkFunctionLibrary.Stop
local StartTimer = UE.UUnLuaBenchmarkFunctionLibrary.StartTimer
local StopTimer = UE.UUnLuaBenchmarkFunctionLibrary.StopTimer

function M:Run(N)
	local SpawnClass = UE.AUnLuaBenchmarkProxy
	local Transform = UE.FTransform()
	local AlwaysSpawn = UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn
	local Proxy = self:GetWorld():SpawnActor(SpawnClass, Transform, AlwaysSpawn)

	-- warm up
	for i=1, N do
		Proxy:NOP()
	end
	
	StartTimer("read int32")
	for i=1, N do
		local MeshID = Proxy.MeshID
	end
	StopTimer()

	StartTimer("write int32")
	for i=1, N do
		Proxy.MeshID = i
	end
	StopTimer()

	StartTimer("read FString")
	for i=1, N do
		local MeshName = Proxy.MeshName
	end
	StopTimer()

	StartTimer("write FString")
	for i=1, N do
		Proxy.MeshName = "9527"
	end
	StopTimer()

	StartTimer("read FVector")
	for i=1, N do
		local COM = Proxy.COM
	end
	StopTimer()

	local COM = UE.FVector(1.0, 1.0, 1.0)
	StartTimer("write FVector")
	for i=1, N do
		Proxy.COM = COM
	end
	StopTimer()

	StartTimer("read TArray<FVector>")
	for i=1, N do
		local Positions = Proxy.Positions
	end
	StopTimer()

	local PredictedPositions = UE.TArray(UE.FVector)
	StartTimer("write TArray<FVector>")
	for i=1, N do
		Proxy.PredictedPositions = PredictedPositions
	end
	StopTimer()

	StartTimer("void NOP()")
	for i=1, N do
		Proxy:NOP()
	end
	StopTimer()

	StartTimer("void Simulate(float)")
	for i=1, N do
		Proxy:Simulate(0.0167)
	end
	StopTimer()

	StartTimer("int32 GetMeshID() const")
	for i=1, N do
		local MeshID = Proxy:GetMeshID()
	end
	StopTimer()

	StartTimer("const FString& GetMeshName() const")
	for i=1, N do
		local MeshName = Proxy:GetMeshName()
	end
	StopTimer()

	StartTimer("const FVector& GetCOM()")
	for i=1, N do
		Proxy:GetCOM(COM)
	end
	StopTimer()

	StartTimer("int32 UpdateMeshID(int32)")
	for i=1, N do
		local NewMeshID = Proxy:UpdateMeshID(1024)
	end
	StopTimer()

	StartTimer("FString UpdateMeshName(const FString&)")
	for i=1, N do
		local NewMeshName = Proxy:UpdateMeshName("1024")
	end
	StopTimer()

	local Origin = UE.FVector(1.0, 1.0, 1.0)
	local Direction = UE.FVector(1.0, 0.0, 0.0)
	StartTimer("bool Raycast(const FVector&, const FVector&) const")
	for i=1, N do
		local bHit = Proxy:Raycast(Origin, Direction)
	end
	StopTimer()

	local Indices = UE.TArray(0)
	StartTimer("void GetIndices(TArray<int32>&) const")
	for i=1, N do
		Proxy:GetIndices(Indices)
	end
	StopTimer()

	for i=1, 1024 do
		Indices:Add(i)
	end
	StartTimer("void GetIndices(TArray<int32>&) const with 1024 items")
	for i=1, N do
		Proxy:GetIndices(Indices)
	end
	StopTimer()

	StartTimer("void UpdateIndices(const TArray<int32>&)")
	for i=1, N do
		Proxy:UpdateIndices(Indices)
	end
	StopTimer()

	local Positions = UE.TArray(UE.FVector)
	StartTimer("void GetPositions(TArray<FVector>&) const")
	for i=1, N do
		Proxy:GetPositions(Positions)
	end
	StopTimer()

	for i=1, 1024 do
		Positions:Add(UE.FVector(i, i, i))
	end
	StartTimer("void GetPositions(TArray<FVector>&) const with 1024 items")
	for i=1, N do
		Proxy:GetPositions(Positions)
	end
	StopTimer()

	StartTimer("void UpdatePositions(const TArray<FVector>&)")
	for i=1, N do
		Proxy:UpdatePositions(Positions)
	end
	StopTimer()

	StartTimer("const TArray<FVector>& GetPredictedPositions() const")
	for i=1, N do
		Proxy:GetPredictedPositions(PredictedPositions)
	end
	StopTimer()

	StartTimer("bool GetMeshInfo(int32&, FString&, FVector&, TArray<int32>&, TArray<FVector>&, TArray<FVector>&) const")
	for i=1, N do
		local bResult, ID, Name = Proxy:GetMeshInfo(0, "", COM, Indices, Positions, PredictedPositions)
	end
	StopTimer()

	StartTimer("FHitResult()")
	local FHitResult = UE.FHitResult
	for i=1, N do
		local HitResult = FHitResult()
	end
	StopTimer()
end

return M
