// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FUnLuaIntelliSenseBPCommands : public TCommands<FUnLuaIntelliSenseBPCommands>
{
public:

	FUnLuaIntelliSenseBPCommands()
		: TCommands<FUnLuaIntelliSenseBPCommands>(TEXT("UnLuaIntelliSenseBP"),
			NSLOCTEXT("Contexts", "UnLuaIntelliSenseBP", "UnLuaIntelliSenseBP Tools"), 
			NAME_None, 
			FEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > UpdateAllBP;
};