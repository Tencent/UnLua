require "UnLua"

local UnLuaPerformanceTestProxy = Class()

function UnLuaPerformanceTestProxy:ReceiveBeginPlay()
	local N = 1000000
	local Multiplier = 1000000000.0 / N
	local RawObject = self.Object

	-- warm up
	for i=1, N do
		self:NOP()
	end
	
	local StartTime = Seconds()
	for i=1, N do
		local MeshID = RawObject.MeshID
	end
	local EndTime = Seconds()
	local Message = "read int32 ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		RawObject.MeshID = i
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "write int32 ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local MeshName = RawObject.MeshName
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "read FString ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		RawObject.MeshName = "9527"
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "write FString ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local COM = RawObject.COM
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "read FVector ; " .. tostring((EndTime - StartTime) * Multiplier)

	local COM = UE4.FVector(1.0, 1.0, 1.0)
	StartTime = Seconds()
	for i=1, N do
		RawObject.COM = COM
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "write FVector ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local Positions = RawObject.Positions
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "read TArray<FVector> ; " .. tostring((EndTime - StartTime) * Multiplier)

	local PredictedPositions = UE4.TArray(UE4.FVector)
	StartTime = Seconds()
	for i=1, N do
		RawObject.PredictedPositions = PredictedPositions
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "write TArray<FVector> ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:NOP()
	end
	EndTime = Seconds()
	Message = Message .. "\n" ..  "void NOP() ; " .. tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:Simulate(0.0167)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void Simulate(float) ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local MeshID = self:GetMeshID()
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "int32 GetMeshID() const ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local MeshName = self:GetMeshName()
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "const FString& GetMeshName() const ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:GetCOM(COM)
		--local COMCopy = self:GetCOM(COM)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "const FVector& GetCOM() const ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local NewMeshID = self:UpdateMeshID(1024)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "int32 UpdateMeshID(int32) ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local NewMeshName = self:UpdateMeshName("1024")
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "FString UpdateMeshName(const FString&) ; "..tostring((EndTime - StartTime) * Multiplier)

	local Origin = UE4.FVector(1.0, 1.0, 1.0)
	local Direction = UE4.FVector(1.0, 0.0, 0.0)
	StartTime = Seconds()
	for i=1, N do
		local bHit = self:Raycast(Origin, Direction)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "bool Raycast(const FVector&, const FVector&) const ; "..tostring((EndTime - StartTime) * Multiplier)

	local Indices = UE4.TArray(0)
	StartTime = Seconds()
	for i=1, N do
		self:GetIndices(Indices)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void GetIndices(TArray<int32>&) const ; "..tostring((EndTime - StartTime) * Multiplier)

	for i=1, 1024 do
		Indices:Add(i)
	end
	StartTime = Seconds()
	for i=1, N do
		self:GetIndices(Indices)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void GetIndices(TArray<int32>&) const with 1024 items ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:UpdateIndices(Indices)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void UpdateIndices(const TArray<int32>&) ; "..tostring((EndTime - StartTime) * Multiplier)

	local Positions = UE4.TArray(UE4.FVector)
	StartTime = Seconds()
	for i=1, N do
		self:GetPositions(Positions)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void GetPositions(TArray<FVector>&) const ; "..tostring((EndTime - StartTime) * Multiplier)

	for i=1, 1024 do
		Positions:Add(UE4.FVector(i, i, i))
	end
	StartTime = Seconds()
	for i=1, N do
		self:GetPositions(Positions)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void GetPositions(TArray<FVector>&) const with 1024 items ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:UpdatePositions(Positions)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "void UpdatePositions(const TArray<FVector>&) ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		self:GetPredictedPositions(PredictedPositions)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "const TArray<FVector>& GetPredictedPositions() const ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local ID, Name, bResult = self:GetMeshInfo(0, "", COM, Indices, Positions, PredictedPositions)
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "bool GetMeshInfo(int32&, FString&, FVector&, TArray<int32>&, TArray<FVector>&, TArray<FVector>&) const ; "..tostring((EndTime - StartTime) * Multiplier)

	StartTime = Seconds()
	for i=1, N do
		local HitResult = UE4.FHitResult()
	end
	EndTime = Seconds()
	Message = Message .. "\n" .. "FHitResult() ; "..tostring((EndTime - StartTime) * Multiplier)

	LogPerformanceData(Message)
end

return UnLuaPerformanceTestProxy
