#include "UnLuaBase.h"
#include "BlueprintToolbar.h"
#include "BlueprintEditorModule.h"

void FBlueprintToolbar::Initialize()
{
	FUnLuaEditorToolbar::Initialize();

	auto& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	auto& ExtenderDelegates = BlueprintEditorModule.GetMenuExtensibilityManager()->GetExtenderDelegates();
	ExtenderDelegates.Add(FAssetEditorExtender::CreateLambda(
		[&](const TSharedRef<FUICommandList>, const TArray<UObject*> ContextSensitiveObjects)
		{
			const auto TargetObject = ContextSensitiveObjects.Num() < 1 ? nullptr : Cast<UBlueprint>(ContextSensitiveObjects[0]);
			return GetExtender(TargetObject);
		}));
}
