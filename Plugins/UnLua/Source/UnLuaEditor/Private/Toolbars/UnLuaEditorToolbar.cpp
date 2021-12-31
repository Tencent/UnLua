#include "UnLuaPrivate.h"
#include "UnLuaEditorCore.h"
#include "UnLuaEditorToolbar.h"
#include "UnLuaEditorCommands.h"
#include "UnLuaInterface.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"
#include "BlueprintEditor.h"
#include "SBlueprintEditorToolbar.h"
#include "Framework/Docking/SDockingTabWell.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Layout/Children.h"

#define LOCTEXT_NAMESPACE "FUnLuaEditorModule"

FUnLuaEditorToolbar::FUnLuaEditorToolbar()
    : CommandList(new FUICommandList)
{
}

void FUnLuaEditorToolbar::Initialize()
{
    BindCommands();
}

void FUnLuaEditorToolbar::BindCommands()
{
    const auto& Commands = FUnLuaEditorCommands::Get();
    CommandList->MapAction(Commands.CreateLuaTemplate, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::CreateLuaTemplate_Executed));
    CommandList->MapAction(Commands.CopyAsRelativePath, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::CopyAsRelativePath_Executed));
    CommandList->MapAction(Commands.BindToLua, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::BindToLua_Executed));
    CommandList->MapAction(Commands.UnbindFromLua, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::UnbindFromLua_Executed));
}

void FUnLuaEditorToolbar::BuildToolbar(FToolBarBuilder& ToolbarBuilder, UObject* InContextObject)
{
    if (!InContextObject)
        return;

    ToolbarBuilder.BeginSection(NAME_None);

    const auto Blueprint = Cast<UBlueprint>(InContextObject);
    const auto BindingStatus = GetBindingStatus(Blueprint);
    FString InStyleName;
    switch (BindingStatus)
    {
    case NotBound:
        InStyleName = "UnLuaEditor.Status_NotBound";
        break;
    case Bound:
        InStyleName = "UnLuaEditor.Status_Bound";
        break;
    case BoundButInvalid:
        InStyleName = "UnLuaEditor.Status_BoundButInvalid";
        break;
    default:
        check(false);
    }
    UE_LOG(LogUnLua, Log, TEXT("InStyleName=%s"), *InStyleName);

    ToolbarBuilder.AddComboButton(
        FUIAction(),
        FOnGetContent::CreateLambda([&, BindingStatus, InContextObject]()
        {
            ContextObject = InContextObject;
            const FUnLuaEditorCommands& Commands = FUnLuaEditorCommands::Get();
            FMenuBuilder MenuBuilder(true, CommandList);
            if (BindingStatus == NotBound)
            {
                MenuBuilder.AddMenuEntry(Commands.BindToLua);
            }
            else
            {
                MenuBuilder.AddMenuEntry(Commands.CopyAsRelativePath);
                MenuBuilder.AddMenuEntry(Commands.CreateLuaTemplate);
                MenuBuilder.AddMenuEntry(Commands.UnbindFromLua);
            }
            return MenuBuilder.MakeWidget();
        }),
        LOCTEXT("UnLua_Label", "UnLua"),
        LOCTEXT("UnLua_ToolTip", "UnLua"),
        FSlateIcon("UnLuaEditorStyle", *InStyleName)
    );

    ToolbarBuilder.EndSection();
}

TSharedRef<FExtender> FUnLuaEditorToolbar::GetExtender(UObject* InContextObject)
{
    TSharedRef<FExtender> ToolbarExtender(new FExtender());
    const auto ExtensionDelegate = FToolBarExtensionDelegate::CreateLambda([this, InContextObject](FToolBarBuilder& ToolbarBuilder)
    {
        BuildToolbar(ToolbarBuilder, InContextObject);
    });
    ToolbarExtender->AddToolBarExtension("Debugging", EExtensionHook::After, CommandList, ExtensionDelegate);
    return ToolbarExtender;
}

void FUnLuaEditorToolbar::BindToLua_Executed() const
{
    const auto Blueprint = Cast<UBlueprint>(ContextObject);
    if (!IsValid(Blueprint))
        return;

    const auto TargetClass = Blueprint->GeneratedClass;
    if (!IsValid(TargetClass))
        return;

    if (TargetClass->ImplementsInterface(UUnLuaInterface::StaticClass()))
        return;

    const auto Ok = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, FName("UnLuaInterface"));
    if (!Ok)
        return;

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)

    const auto BlueprintEditors = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet").GetBlueprintEditors();
    for (auto BlueprintEditor : BlueprintEditors)
    {
        const auto MyBlueprintEditor = static_cast<FBlueprintEditor*>(&BlueprintEditors[0].Get());
        if (!MyBlueprintEditor || MyBlueprintEditor->GetBlueprintObj() != Blueprint)
            continue;
        MyBlueprintEditor->Compile();

        const auto Func = Blueprint->GeneratedClass->FindFunctionByName(FName("GetModuleName"));
        const auto GraphToOpen = FBlueprintEditorUtils::FindScopeGraph(Blueprint, Func);
        MyBlueprintEditor->OpenGraphAndBringToFront(GraphToOpen);
    }

#endif
}

void FUnLuaEditorToolbar::UnbindFromLua_Executed() const
{
    const auto Blueprint = Cast<UBlueprint>(ContextObject);
    if (!IsValid(Blueprint))
        return;

    const auto TargetClass = Blueprint->GeneratedClass;
    if (!IsValid(TargetClass))
        return;

    if (!TargetClass->ImplementsInterface(UUnLuaInterface::StaticClass()))
        return;

    FBlueprintEditorUtils::RemoveInterface(Blueprint, FName("UnLuaInterface"));

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)

    const auto BlueprintEditors = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet").GetBlueprintEditors();
    for (auto BlueprintEditor : BlueprintEditors)
    {
        const auto MyBlueprintEditor = static_cast<FBlueprintEditor*>(&BlueprintEditors[0].Get());
        if (!MyBlueprintEditor || MyBlueprintEditor->GetBlueprintObj() != Blueprint)
            continue;
        MyBlueprintEditor->Compile();
        MyBlueprintEditor->RefreshEditors();
    }

#endif

    const auto ActiveTab = FGlobalTabmanager::Get()->GetActiveTab();
    if (!ActiveTab)
        return;

    const auto DockingTabWell = ActiveTab->GetParent();
    if (!DockingTabWell)
        return;

    const auto DockTabs = DockingTabWell->GetChildren(); // DockingTabWell->GetTabs(); 
    for (auto i = 0; i < DockTabs->Num(); i++)
    {
        const auto DockTab = StaticCastSharedRef<SDockTab>(DockTabs->GetChildAt(i));
        const auto Label = DockTab->GetTabLabel();
        if (Label.ToString().Equals("$$ Get Module Name $$"))
        {
            DockTab->RequestCloseTab();
        }
    }
}

void FUnLuaEditorToolbar::CreateLuaTemplate_Executed()
{
    const auto Blueprint = Cast<UBlueprint>(ContextObject);
    if (!IsValid(Blueprint))
        return;

    UClass* Class = Blueprint->GeneratedClass;

    const auto Func = Class->FindFunctionByName(FName("GetModuleName"));
    if (!IsValid(Func))
        return;

    FString ModuleName;
    Class->ProcessEvent(Func, &ModuleName);

    if (ModuleName.IsEmpty())
        return;

    TArray<FString> ModuleNameParts;
    ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
    const auto ClassName = ModuleNameParts.Last();

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/"));
    const auto FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *RelativePath);

    if (FPaths::FileExists(FileName))
    {
        UE_LOG(LogUnLua, Warning, TEXT("Lua file (%s) is already existed!"), *ClassName);
        return;
    }

    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();

    FString TemplateName;
    if (Class->IsChildOf(AActor::StaticClass()))
    {
        // default BlueprintEvents for Actor
        TemplateName = ContentDir + TEXT("/ActorTemplate.lua");
    }
    else if (Class->IsChildOf(UUserWidget::StaticClass()))
    {
        // default BlueprintEvents for UserWidget (UMG)
        TemplateName = ContentDir + TEXT("/UserWidgetTemplate.lua");
    }
    else if (Class->IsChildOf(UAnimInstance::StaticClass()))
    {
        // default BlueprintEvents for AnimInstance (animation blueprint)
        TemplateName = ContentDir + TEXT("/AnimInstanceTemplate.lua");
    }
    else if (Class->IsChildOf(UActorComponent::StaticClass()))
    {
        // default BlueprintEvents for ActorComponent
        TemplateName = ContentDir + TEXT("/ActorComponentTemplate.lua");
    }

    FString Content;
    FFileHelper::LoadFileToString(Content, *TemplateName);
    Content = Content.Replace(TEXT("TemplateName"), *ClassName);

    FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void FUnLuaEditorToolbar::CopyAsRelativePath_Executed() const
{
    const auto Blueprint = Cast<UBlueprint>(ContextObject);
    if (!IsValid(Blueprint))
        return;

    const auto TargetClass = Blueprint->GeneratedClass;
    if (!IsValid(TargetClass))
        return;

    if (!TargetClass->ImplementsInterface(UUnLuaInterface::StaticClass()))
        return;

    const auto Func = TargetClass->FindFunctionByName(FName("GetModuleName"));
    if (!IsValid(Func))
        return;

    FString ModuleName;
    const auto DefaultObject = TargetClass->GetDefaultObject();
    DefaultObject->UObject::ProcessEvent(Func, &ModuleName);

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/")) + TEXT(".lua");
    FPlatformApplicationMisc::ClipboardCopy(*RelativePath);
}

#undef LOCTEXT_NAMESPACE
