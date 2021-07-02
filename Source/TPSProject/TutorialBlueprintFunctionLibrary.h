#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TutorialBlueprintFunctionLibrary.generated.h"

UCLASS()
class TPSPROJECT_API UTutorialBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CallLuaByGlobalTable", Category = "UnLua Tutorial"))
	static void CallLuaByGlobalTable();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CallLuaByFLuaTable", Category = "UnLua Tutorial"))
	static void CallLuaByFLuaTable();
};