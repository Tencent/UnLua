// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnLuaIntelliSenseBPCommands.h"

#define LOCTEXT_NAMESPACE "FUnLuaIntelliSenseModuleBP"

void FUnLuaIntelliSenseBPCommands::RegisterCommands()
{
	UI_COMMAND(UpdateAllBP, "Update BP IntelliSense", "Update all blueprints intellisense data.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
