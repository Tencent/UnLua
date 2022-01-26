#include "UnLuaBase.h"
#include "AnimationBlueprintToolbar.h"
#include "IAnimationBlueprintEditorModule.h"
#include "AnimationBlueprintEditorModule.h"
#include "IPersonaToolkit.h"
#include "Animation/AnimBlueprint.h"

void FAnimationBlueprintToolbar::Initialize()
{
	FUnLuaEditorToolbar::Initialize();

	auto& AnimationBlueprintEditorModule = FModuleManager::LoadModuleChecked<FAnimationBlueprintEditorModule>("AnimationBlueprintEditor");
	auto& ToolbarExtenders = AnimationBlueprintEditorModule.GetAllAnimationBlueprintEditorToolbarExtenders();
	ToolbarExtenders.Add(IAnimationBlueprintEditorModule::FAnimationBlueprintEditorToolbarExtender::CreateLambda(
		[&](const TSharedRef<FUICommandList>, const TSharedRef<IAnimationBlueprintEditor> Editor)
		{
			const auto TargetObject = Editor->GetPersonaToolkit()->GetAnimBlueprint(); 
			return GetExtender(TargetObject);
		}));
}
