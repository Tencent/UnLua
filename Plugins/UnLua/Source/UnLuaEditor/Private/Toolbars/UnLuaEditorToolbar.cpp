#include "Misc/EngineVersionComparison.h"
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
#include "LuaModuleLocator.h"
#include "SBlueprintEditorToolbar.h"
#include "Framework/Docking/SDockingTabWell.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Layout/Children.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ToolMenus.h"
#include "UnLuaSettings.h"
#include "UnLuaIntelliSense.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Serialization/JsonSerializer.h"

#define LOCTEXT_NAMESPACE "FUnLuaEditorModule"

FUnLuaEditorToolbar::FUnLuaEditorToolbar()
    : CommandList(new FUICommandList),
      ContextObject(nullptr)
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
    CommandList->MapAction(Commands.CreateDefaultLuaTemplate, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::CreateLuaDefualtTemplate_Executed));
    CommandList->MapAction(Commands.CopyAsDefaultModuleName, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::CopyAsDefualtModuleName_Executed));
    CommandList->MapAction(Commands.BindToLua, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::BindToLua_Executed));
    CommandList->MapAction(Commands.UnbindFromLua, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::UnbindFromLua_Executed));
    CommandList->MapAction(Commands.RevealInExplorer, FExecuteAction::CreateRaw(this, &FUnLuaEditorToolbar::RevealInExplorer_Executed));
}

void FUnLuaEditorToolbar::BuildToolbar(FToolBarBuilder& ToolbarBuilder, UObject* InContextObject)
{
    if (!InContextObject)
        return;

    ToolbarBuilder.BeginSection(NAME_None);

    const auto Blueprint = Cast<UBlueprint>(InContextObject);
    ToolbarBuilder.AddComboButton(
        FUIAction(),
        FOnGetContent::CreateLambda([&, Blueprint, InContextObject]()
        {
            ContextObject = InContextObject;
            const auto BindingStatus = GetBindingStatus(Blueprint);
            const FUnLuaEditorCommands& Commands = FUnLuaEditorCommands::Get();
            FMenuBuilder MenuBuilder(true, CommandList);
            if (BindingStatus == NotBound)
            {
                MenuBuilder.AddMenuEntry(Commands.BindToLua, NAME_None, LOCTEXT("Bind", "Bind"));
            }
            else
            {
				MenuBuilder.AddMenuEntry(Commands.CopyAsRelativePath, NAME_None, LOCTEXT("CopyAsRelativePath", "Copy as Relative Path"));
                MenuBuilder.AddMenuEntry(Commands.RevealInExplorer, NAME_None, LOCTEXT("RevealInExplorer", "Reveal in Explorer"));
				MenuBuilder.AddMenuEntry(Commands.CreateLuaTemplate, NAME_None, LOCTEXT("CreateLuaTemplate", "Create Lua Template"));
                MenuBuilder.AddMenuEntry(Commands.CopyAsDefaultModuleName, NAME_None, LOCTEXT("CopyAsDefaultModuleName", "Copy as Default Module Name"));
				MenuBuilder.AddMenuEntry(Commands.CreateDefaultLuaTemplate, NAME_None, LOCTEXT("CreateDefaultLuaTemplate", "Create Default Lua Template"));
                MenuBuilder.AddMenuEntry(Commands.UnbindFromLua, NAME_None, LOCTEXT("Unbind", "Unbind"));
            }
            return MenuBuilder.MakeWidget();
        }),
        LOCTEXT("UnLua_Label", "UnLua"),
        LOCTEXT("UnLua_ToolTip", "UnLua"),
        TAttribute<FSlateIcon>::Create([Blueprint]
        {
            const auto BindingStatus = GetBindingStatus(Blueprint);
            FString InStyleName;
            switch (BindingStatus)
            {
            case Unknown:
                InStyleName = "UnLuaEditor.Status_Unknown";
                break;
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

            return FSlateIcon("UnLuaEditorStyle", *InStyleName);
        })
    );

    ToolbarBuilder.EndSection();

    BuildNodeMenu();
}

void FUnLuaEditorToolbar::BuildNodeMenu()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* BPMenu = UToolMenus::Get()->ExtendMenu("GraphEditor.GraphNodeContextMenu.K2Node_FunctionResult");
    BPMenu->AddDynamicSection("UnLua", FNewToolMenuDelegate::CreateLambda([this](UToolMenu* ToolMenu)
    {
        UGraphNodeContextMenuContext* GraphNodeCtx = ToolMenu->FindContext<UGraphNodeContextMenuContext>();
        if (GraphNodeCtx && GraphNodeCtx->Graph)
        {
            if (GraphNodeCtx->Graph->GetName() == "GetModuleName")
            {
                FToolMenuSection& UnLuaSection = ToolMenu->AddSection("UnLua", FText::FromString("UnLua"));
                UnLuaSection.AddEntry(FToolMenuEntry::InitMenuEntryWithCommandList(FUnLuaEditorCommands::Get().RevealInExplorer, CommandList, LOCTEXT("RevealInExplorer", "Reveal in Explorer")));
            }
        }
    }), FToolMenuInsert(NAME_None, EToolMenuInsertType::First));
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

#if UE_VERSION_OLDER_THAN(5, 1, 0)
    const auto Ok = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, FName("UnLuaInterface"));
#else
    const auto Ok = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, FTopLevelAssetPath(UUnLuaInterface::StaticClass()));
#endif
    if (!Ok)
        return;

    FString LuaModuleName;
    const auto ModifierKeys = FSlateApplication::Get().GetModifierKeys();
    const auto bIsAltDown = ModifierKeys.IsLeftAltDown() || ModifierKeys.IsRightAltDown();
    if (bIsAltDown)
    {
        const auto Package = Blueprint->GetTypedOuter(UPackage::StaticClass());
        LuaModuleName = Package->GetName().RightChop(6).Replace(TEXT("/"), TEXT("."));
    }
    else
    {
        const auto Settings = GetDefault<UUnLuaSettings>();
        if (Settings && Settings->ModuleLocatorClass)
        {
            const auto ModuleLocator = Cast<ULuaModuleLocator>(Settings->ModuleLocatorClass->GetDefaultObject());
            LuaModuleName = ModuleLocator->Locate(TargetClass);
        }
    }

    if (!LuaModuleName.IsEmpty())
    {
        const auto InterfaceDesc = *Blueprint->ImplementedInterfaces.FindByPredicate([](const FBPInterfaceDescription& Desc)
        {
            return Desc.Interface == UUnLuaInterface::StaticClass();
        });
        InterfaceDesc.Graphs[0]->Nodes[1]->Pins[1]->DefaultValue = LuaModuleName;
    }

#if !UE_VERSION_OLDER_THAN(4, 26, 0)

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

#if UE_VERSION_OLDER_THAN(5, 1, 0)
    FBlueprintEditorUtils::RemoveInterface(Blueprint, FName("UnLuaInterface"));
#else
    FBlueprintEditorUtils::RemoveInterface(Blueprint, FTopLevelAssetPath(UUnLuaInterface::StaticClass()));
#endif

#if !UE_VERSION_OLDER_THAN(4, 26, 0)

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
    Class->GetDefaultObject()->ProcessEvent(Func, &ModuleName);

    if (ModuleName.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("ModuleNameRequired", "Please specify a module name first"));
        Info.ExpireDuration = 5;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    TArray<FString> ModuleNameParts;
    ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
    const auto TemplateName = ModuleNameParts.Last();

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/"));
    const auto FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *RelativePath);

    if (FPaths::FileExists(FileName))
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s"), *FText::Format(LOCTEXT("FileAlreadyExists", "Lua file ({0}) is already existed!"), FText::FromString(TemplateName)).ToString());
        return;
    }

    static FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetBaseDir();
    for (auto TemplateClass = Class; TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
    {
        auto TemplateClassName = TemplateClass->GetName().EndsWith("_C") ? TemplateClass->GetName().LeftChop(2) : TemplateClass->GetName();
        auto RelativeFilePath = "Config/LuaTemplates" / TemplateClassName + ".lua";
        auto FullFilePath = FPaths::ProjectDir() / RelativeFilePath;
        if (!FPaths::FileExists(FullFilePath))
            FullFilePath = BaseDir / RelativeFilePath;

        if (!FPaths::FileExists(FullFilePath))
            continue;

        FString Content;
        FFileHelper::LoadFileToString(Content, *FullFilePath);
        Content = Content.Replace(TEXT("TemplateName"), *TemplateName)
                         .Replace(TEXT("ClassName"), *UnLua::IntelliSense::GetTypeName(Class));

        FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        break;
    }
}

void FUnLuaEditorToolbar::RevealInExplorer_Executed()
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

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/"));
    const auto FileName = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *RelativePath);

    if (IFileManager::Get().FileExists(*FileName))
    {
        FPlatformProcess::ExploreFolder(*FileName);
    }
    else
    {
        FNotificationInfo NotificationInfo(FText::FromString("UnLua Notification"));
        NotificationInfo.Text = LOCTEXT("FileNotExist", "The file does not exist.");
        NotificationInfo.bFireAndForget = true;
        NotificationInfo.ExpireDuration = 100.0f;
        NotificationInfo.bUseThrobber = true;
        FSlateNotificationManager::Get().AddNotification(NotificationInfo);
    }
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

void FUnLuaEditorToolbar::CreateLuaDefualtTemplate_Executed()
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
		return;

	const UClass* Class = Blueprint->GeneratedClass;
	const FString ClassName = Class->GetName();
	FString OuterPath = Class->GetPathName();

    int32 LastIndex;
    if (OuterPath.FindLastChar('/', LastIndex))
    {
        OuterPath = OuterPath.Left(LastIndex + 1);
    }

    OuterPath = OuterPath.RightChop(1);
    int32 FirstIndex;
    if (OuterPath.FindChar('/', FirstIndex))
    {
        OuterPath = OuterPath.RightChop(FirstIndex + 1);
    }
    
	const FString FileName = FString::Printf(TEXT("%s%s%s.lua"), *GLuaSrcFullPath, *OuterPath, *ClassName);
	if (FPaths::FileExists(FileName))
	{
		UE_LOG(LogUnLua, Warning, TEXT("Lua file (%s) is already existed!"), *ClassName);
		return;
	}

    const UClass* UnLuaSuperClass = nullptr;
    for (auto TemplateClass = Class->GetSuperClass(); TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
    {
        if (TemplateClass->ImplementsInterface(UUnLuaInterface::StaticClass()))
        {
            UnLuaSuperClass = TemplateClass;
            break;
        }
    }

	static FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetBaseDir();
	for (auto TemplateClass = Class; TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
	{
		auto TemplateClassName = TemplateClass->GetName().EndsWith("_C") ? TemplateClass->GetName().LeftChop(2) : TemplateClass->GetName();
		auto RelativeFilePath = "Config/LuaTemplates" / TemplateClassName + ".lua";
		auto FullFilePath = FPaths::ProjectDir() / RelativeFilePath;
		if (!FPaths::FileExists(FullFilePath))
			FullFilePath = BaseDir / RelativeFilePath;

		if (!FPaths::FileExists(FullFilePath))
			continue;

        FString CompanyName = "";
        FString AuthorName = "";
        static FString UnLuaFilename = FPaths::ProjectSavedDir() / TEXT("UnLua.json");
        if (FPaths::FileExists(UnLuaFilename))
        {
            FString JsonContent;
            FFileHelper::LoadFileToString(JsonContent, *UnLuaFilename);
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
            TSharedPtr<FJsonObject> JsonObject;
            if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
            {
                if (JsonObject->HasField(TEXT("company")))
                {
                    JsonObject->TryGetStringField(TEXT("company"), CompanyName);
                }

                if (JsonObject->HasField(TEXT("author")))
                {
                    JsonObject->TryGetStringField(TEXT("author"), AuthorName);
                }
            }
        }

		FString Content;
		FFileHelper::LoadFileToString(Content, *FullFilePath);
		Content = Content.Replace(TEXT("TemplateName"), *ClassName)
						 .Replace(TEXT("ClassName"), *UnLua::IntelliSense::GetTypeName(Class))
                         .Replace(TEXT("@COMPANY **"), *FString::Printf(TEXT("@COMPANY %s"), *CompanyName))
                         .Replace(TEXT("@AUTHOR **"), *FString::Printf(TEXT("@AUTHOR %s"), *AuthorName))
                         .Replace(TEXT("${date}"), *FDateTime::Now().ToString(TEXT("%Y/%m/%d")))
                         .Replace(TEXT("${time}"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")));
        
        if (UnLuaSuperClass)
        {
            const FString SuperClassName = UnLuaSuperClass->GetName();
            FString SuperPath = UnLuaSuperClass->GetPathName();
            int32 SuperLastIndex;
            if (SuperPath.FindLastChar('/', SuperLastIndex))
            {
                SuperPath = SuperPath.Left(SuperLastIndex + 1);
            }
            SuperPath = SuperPath.RightChop(6); // ignore "/Game/"
            const FString SuperFileName = FString::Printf(TEXT("%s%s"), *SuperPath, *SuperClassName).Replace(TEXT("/"), TEXT("."));

            Content = Content.Replace(TEXT(" = Class()"), *FString::Printf(TEXT(", Super = Class(\"%s\")"), *SuperFileName))
                             .Replace(TEXT("self.Overridden"), TEXT("Super"));
        }

		FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		break;
	}
}

void FUnLuaEditorToolbar::CopyAsDefualtModuleName_Executed() const
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
		return;

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
		return;

	if (!TargetClass->ImplementsInterface(UUnLuaInterface::StaticClass()))
		return;

	const UClass* Class = Blueprint->GeneratedClass;
	const FString ClassName = Class->GetName();
	FString OuterPath = Class->GetPathName();
	int32 LastIndex;
	if (OuterPath.FindLastChar('/', LastIndex))
	{
		OuterPath = OuterPath.Left(LastIndex + 1);
	}

    OuterPath = OuterPath.RightChop(1);
    int32 FirstIndex;
    if (OuterPath.FindChar('/', FirstIndex))
    {
        OuterPath = OuterPath.RightChop(FirstIndex + 1);
    }

	const FString FileName = FString::Printf(TEXT("%s%s.lua"), *OuterPath, *ClassName);

	const auto RelativePath = FileName.Replace(TEXT("/"), TEXT("."));
	const auto ReturnRelativePath = RelativePath.Replace(TEXT(".lua"), TEXT(""));
	FPlatformApplicationMisc::ClipboardCopy(*ReturnRelativePath);
}

#undef LOCTEXT_NAMESPACE
