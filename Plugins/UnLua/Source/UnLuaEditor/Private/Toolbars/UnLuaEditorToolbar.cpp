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
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] Blueprint is not valid"));
        return;
    }

    UE_LOG(LogUnLua, Display, TEXT("[UnLua] Blueprint name: %s"), *Blueprint->GetName());

    UClass* Class = Blueprint->GeneratedClass;
    if (!IsValid(Class))
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] Blueprint generated class is not valid"));
        return;
    }

    UE_LOG(LogUnLua, Display, TEXT("[UnLua] Blueprint generated class name: %s"), *Class->GetName());

    const auto Func = Class->FindFunctionByName(FName("GetModuleName"));
    if (!IsValid(Func))
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] GetModuleName function not found"));
        return;
    }

    FString ModuleName;
    Class->GetDefaultObject()->ProcessEvent(Func, &ModuleName);

    if (ModuleName.IsEmpty())
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] Module name is empty"));
        FNotificationInfo Info(LOCTEXT("ModuleNameRequired", "Please specify a module name first"));
        Info.ExpireDuration = 5;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    TArray<FString> ModuleNameParts;
    ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
    const auto TemplateName = ModuleNameParts.Last();

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/"));
    
    // 获取蓝图资源的完整路径和包路径
    FString BlueprintPath = Blueprint->GetPathName();
    FString BlueprintPackagePath = Blueprint->GetOutermost()->GetName();
    FString AssetPath = Blueprint->GetPathName();
    FString OutermostName = Blueprint->GetOutermost()->GetName();
    FString OuterPathName = Blueprint->GetOutermost()->GetPathName();
    FString OutermostPath = Blueprint->GetOutermost()->GetPackage()->GetPathName();
    FString ClassPath = Blueprint->GeneratedClass->GetPathName();
    FString PackageName = Blueprint->GetOutermost()->GetPackage()->GetName();
    
    // 只保留重要的路径信息日志
    UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Blueprint path: %s"), *BlueprintPath);
    UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Blueprint package path: %s"), *BlueprintPackagePath);
    
    // 获取项目和引擎路径信息
    FString ProjectDir = FPaths::ProjectDir();
    FString ProjectContentDir = FPaths::ProjectContentDir();
    FString PluginsDir = FPaths::ProjectPluginsDir();
    
    // 默认的Lua文件存储路径（项目Content/Script目录）
    FString DefaultScriptPath = FPaths::ConvertRelativePathToFull(GLuaSrcFullPath);
    FString FileName = FString::Printf(TEXT("%s%s.lua"), *DefaultScriptPath, *RelativePath);
    
    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 默认脚本路径: %s"), *DefaultScriptPath);
    
    bool bFoundScriptLocation = false;
    
    // 尝试从路径中提取第一段目录名称，可能是插件名称
    // 例如，从"/AstraAI/Blueprints/Widgets/BP_TEST"提取"AstraAI"
    FString FirstPathSegment;
    if (!BlueprintPackagePath.IsEmpty() && BlueprintPackagePath.StartsWith(TEXT("/")))
    {
        FString TrimmedPath = BlueprintPackagePath.RightChop(1); // 去掉开头的 /
        int32 SlashPos = TrimmedPath.Find(TEXT("/"));
        if (SlashPos != INDEX_NONE)
        {
            FirstPathSegment = TrimmedPath.Left(SlashPos);
            UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] First path segment extracted: %s"), *FirstPathSegment);
        }
    }
    
    // 遍历所有路径，检测插件类型
    FString PluginPathToUse;
    FString PluginPathDesc;
    bool bIsGameFeaturePlugin = false;
    
    // 如果还没找到插件路径，尝试通过第一段目录名查找匹配的插件
    if (PluginPathToUse.IsEmpty() && !FirstPathSegment.IsEmpty())
    {
        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 尝试通过目录名 '%s' 查找插件"), *FirstPathSegment);
        
        // 获取项目中所有插件的信息
        TArray<TSharedRef<IPlugin>> AllPlugins = IPluginManager::Get().GetDiscoveredPlugins();
        
        // 遍历所有插件，查找显示名与FirstPathSegment匹配的插件
        bool bFoundPlugin = false;
        FString MatchedPluginName;
        FString MatchedPluginFolder;
        bool bIsMatchedPluginGameFeature = false;
        TSharedPtr<IPlugin> MatchedPlugin;
        
        for (const TSharedRef<IPlugin>& PluginRef : AllPlugins)
        {
            FString PluginName = PluginRef->GetName();
            FString PluginBaseDir = PluginRef->GetBaseDir();
            FString PluginFolder = FPaths::GetCleanFilename(PluginBaseDir);
            
            // 检查插件名称是否与路径段匹配
            if (PluginName.Equals(FirstPathSegment, ESearchCase::IgnoreCase))
            {
                bFoundPlugin = true;
                MatchedPluginName = PluginName;
                MatchedPluginFolder = PluginFolder;
                MatchedPlugin = PluginRef;
                
                // 检查是否为GameFeature插件
                if (PluginBaseDir.Contains(TEXT("/GameFeatures/")) || PluginBaseDir.Contains(TEXT("\\GameFeatures\\")))
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 找到匹配的GameFeature插件: %s, 目录: %s"), *PluginName, *PluginBaseDir);
                    bIsMatchedPluginGameFeature = true;
                    break;
                }
                else
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 找到匹配的普通插件: %s, 目录: %s"), *PluginName, *PluginBaseDir);
                    bIsMatchedPluginGameFeature = false;
                    break;
                }
            }
        }
        
        // 如果找到了匹配的插件
        if (bFoundPlugin && MatchedPlugin.IsValid())
        {
            // 直接使用插件的实际物理路径，而不是基于名称拼接路径
            FString PluginBaseDir = MatchedPlugin->GetBaseDir();
            
            if (bIsMatchedPluginGameFeature)
            {
                bIsGameFeaturePlugin = true;
                PluginPathToUse = FString::Printf(TEXT("/GameFeatures/%s/%s"), *MatchedPluginName, *BlueprintPackagePath);
                PluginPathDesc = TEXT("通过插件管理器找到的GameFeature插件");
            }
            else
            {
                bIsGameFeaturePlugin = false;
                PluginPathToUse = FString::Printf(TEXT("/Plugins/%s/%s"), *MatchedPluginName, *BlueprintPackagePath);
                PluginPathDesc = TEXT("通过插件管理器找到的普通插件");
            }
            
            // 构建插件的脚本路径 - 使用实际物理路径
            FString PluginScriptDir = FPaths::Combine(PluginBaseDir, TEXT("Content/Script"));
            
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件根目录(真实物理路径): %s"), *PluginBaseDir);
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件脚本目录: %s"), *PluginScriptDir);
            
            // 检查插件目录是否存在
            bool bPluginRootExists = FPaths::DirectoryExists(PluginBaseDir);
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件根目录存在: %s"), bPluginRootExists ? TEXT("是") : TEXT("否"));
            
            if (!bPluginRootExists)
            {
                UE_LOG(LogUnLua, Error, TEXT("[UnLua] 插件目录不存在: %s，将使用默认Content/Script"), *PluginBaseDir);
                bFoundScriptLocation = false;
            }
            else
            {
                // 检查插件Content目录是否存在
                FString PluginContentDir = FPaths::Combine(PluginBaseDir, TEXT("Content"));
                bool bContentDirExists = FPaths::DirectoryExists(PluginContentDir);
                UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件Content目录存在: %s"), bContentDirExists ? TEXT("是") : TEXT("否"));
                
                if (!bContentDirExists)
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件Content目录不存在: %s，尝试创建"), *PluginContentDir);
                    bool bContentDirCreated = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*PluginContentDir);
                    
                    if (!bContentDirCreated)
                    {
                        UE_LOG(LogUnLua, Error, TEXT("[UnLua] 无法创建Content目录，将使用默认Content/Script"));
                        bFoundScriptLocation = false;
                    }
                    else
                    {
                        // Content目录创建成功，继续创建Script目录
                        bool bScriptDirCreated = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*PluginScriptDir);
                        
                        if (!bScriptDirCreated)
                        {
                            UE_LOG(LogUnLua, Warning, TEXT("[UnLua] 无法在插件中创建Script目录: %s"), *PluginScriptDir);
                            bFoundScriptLocation = false;
                        }
                        else
                        {
                            bFoundScriptLocation = true;
                        }
                    }
                }
                else
                {
                    // Content目录存在，检查Script目录
                    bool bScriptDirExists = FPaths::DirectoryExists(PluginScriptDir);
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 插件脚本目录存在: %s"), bScriptDirExists ? TEXT("是") : TEXT("否"));
                    
                    if (!bScriptDirExists)
                    {
                        bool bCreateScriptResult = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*PluginScriptDir);
                        
                        if (!bCreateScriptResult)
                        {
                            UE_LOG(LogUnLua, Warning, TEXT("[UnLua] 无法在插件中创建Script目录: %s"), *PluginScriptDir);
                            bFoundScriptLocation = false;
                        }
                        else
                        {
                            bFoundScriptLocation = true;
                        }
                    }
                    else
                    {
                        // Script目录已存在
                        bFoundScriptLocation = true;
                    }
                }
                
                // 如果找到了脚本位置，更新文件名
                if (bFoundScriptLocation)
                {
                    // 设置新的Lua文件路径
                    FString OldFileName = FileName;
                    if (ModuleNameParts.Num() > 1 && ModuleNameParts[0] == MatchedPluginName)
                    {
                        // 如果模块名已经包含插件名前缀，直接使用相对路径
                        FileName = FPaths::Combine(PluginScriptDir, *RelativePath) + TEXT(".lua");
                        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 使用相对路径创建Lua文件: %s"), *FileName);
                    }
                    else
                    {
                        // 否则，将Lua文件直接放在Script目录下
                        FileName = FPaths::Combine(PluginScriptDir, *TemplateName) + TEXT(".lua");
                        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 使用模板名称创建Lua文件: %s"), *FileName);
                    }
                    
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 文件路径已更新 从: %s 到: %s"), *OldFileName, *FileName);
                }
            }
        }
    }
    
    // 如果找到了插件路径
    if (!PluginPathToUse.IsEmpty())
    {
        if (bIsGameFeaturePlugin)
        {
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 蓝图位于GameFeatures插件中"));
        }
        else
        {
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 蓝图位于普通插件中"));
        }
    }
    else
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] 未找到插件路径"));
    }

    // 检查文件是否已存在
    bool bFileExists = FPaths::FileExists(FileName);
    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 文件已存在: %s"), bFileExists ? TEXT("是") : TEXT("否"));
    
    if (bFileExists)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s"), *FText::Format(LOCTEXT("FileAlreadyExists", "Lua file ({0}) is already existed!"), FText::FromString(TemplateName)).ToString());
        return;
    }

    // 确保目标目录存在
    FString TargetDirectory = FPaths::GetPath(FileName);
    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 目标目录: %s"), *TargetDirectory);
    
    bool bDirExists = FPaths::DirectoryExists(TargetDirectory);
    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 目标目录存在: %s"), bDirExists ? TEXT("是") : TEXT("否"));
    
    if (!bDirExists)
    {
        bool bDirCreated = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*TargetDirectory);
        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 创建目录 %s: %s"), *TargetDirectory, bDirCreated ? TEXT("成功") : TEXT("失败"));
        
        if (!bDirCreated)
        {
            UE_LOG(LogUnLua, Warning, TEXT("[UnLua] 无法创建目录: %s"), *TargetDirectory);
            return;
        }
    }

    // 加载并处理模板
    static FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetBaseDir();
    UE_LOG(LogUnLua, Display, TEXT("[UnLua] UnLua插件基础目录: %s"), *BaseDir);
    
    bool bFoundTemplate = false;
    for (auto TemplateClass = Class; TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
    {
        auto TemplateClassName = TemplateClass->GetName().EndsWith("_C") ? TemplateClass->GetName().LeftChop(2) : TemplateClass->GetName();
        auto RelativeFilePath = "Config/LuaTemplates" / TemplateClassName + ".lua";
        auto FullFilePath = FPaths::ProjectConfigDir() / RelativeFilePath;
        
        UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 尝试模板类: %s"), *TemplateClassName);
        UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 查找项目模板: %s"), *FullFilePath);
        
        if (!FPaths::FileExists(FullFilePath))
        {
            FullFilePath = BaseDir / RelativeFilePath;
            UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 查找插件模板: %s"), *FullFilePath);
        }

        if (!FPaths::FileExists(FullFilePath))
        {
            UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 没有找到模板文件"));
            continue;
        }

        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 找到模板文件: %s"), *FullFilePath);

        FString Content;
        bool bReadSuccess = FFileHelper::LoadFileToString(Content, *FullFilePath);
        UE_LOG(LogUnLua, Display, TEXT("[UnLua] 读取模板文件: %s"), bReadSuccess ? TEXT("成功") : TEXT("失败"));
        
        if (bReadSuccess)
        {
        Content = Content.Replace(TEXT("TemplateName"), *TemplateName)
                         .Replace(TEXT("ClassName"), *UnLua::IntelliSense::GetTypeName(Class));

            bool bFileSaved = FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] 保存Lua文件到 %s: %s"), *FileName, bFileSaved ? TEXT("成功") : TEXT("失败"));
            
            if (bFileSaved)
            {
                UE_LOG(LogUnLua, Log, TEXT("[UnLua] 成功创建Lua模板文件: %s"), *FileName);
                
                // 显示通知
                FNotificationInfo Info(FText::Format(LOCTEXT("FileCreated", "Lua file created: {0}"), FText::FromString(FPaths::GetCleanFilename(FileName))));
                Info.ExpireDuration = 5;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
            else
            {
                UE_LOG(LogUnLua, Error, TEXT("[UnLua] 无法保存Lua文件到: %s"), *FileName);
                
                // 显示错误通知
                FNotificationInfo ErrorInfo(FText::Format(LOCTEXT("FileCreateFailed", "Failed to create Lua file: {0}"), FText::FromString(FPaths::GetCleanFilename(FileName))));
                ErrorInfo.ExpireDuration = 5;
                FSlateNotificationManager::Get().AddNotification(ErrorInfo);
            }
            
            bFoundTemplate = true;
        break;
        }
    }
    
    if (!bFoundTemplate)
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] 没有找到适合的模板文件"));
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

    if (ModuleName.IsEmpty())
        return;

    TArray<FString> ModuleNameParts;
    ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
    const auto TemplateName = ModuleNameParts.Last();

    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/"));
    
    // 获取蓝图资源的完整路径和包路径
    FString BlueprintPath = Blueprint->GetPathName();
    FString BlueprintPackagePath = Blueprint->GetOutermost()->GetName();
    FString AssetPath = Blueprint->GetPathName();
    
    UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Looking for Lua file for blueprint at path: %s"), *BlueprintPath);
    UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Blueprint package path: %s"), *BlueprintPackagePath);
    UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Blueprint asset path: %s"), *AssetPath);
    
    // 默认的Lua文件存储路径（项目Content/Script目录）
    FString DefaultScriptPath = FPaths::ConvertRelativePathToFull(GLuaSrcFullPath);
    FString FileName = FString::Printf(TEXT("%s%s.lua"), *DefaultScriptPath, *RelativePath);
    
    bool bFoundScriptLocation = false;
    TArray<FString> PossibleFileNames;
    
    // 添加默认路径到可能文件位置
    PossibleFileNames.Add(FileName);
    
    // 尝试从路径中提取第一段目录名称，可能是插件名称
    // 例如，从"/AstraAI/Blueprints/Widgets/BP_TEST"提取"AstraAI"
    FString FirstPathSegment;
    if (!BlueprintPackagePath.IsEmpty() && BlueprintPackagePath.StartsWith(TEXT("/")))
    {
        FString TrimmedPath = BlueprintPackagePath.RightChop(1); // 去掉开头的 /
        int32 SlashPos = TrimmedPath.Find(TEXT("/"));
        if (SlashPos != INDEX_NONE)
        {
            FirstPathSegment = TrimmedPath.Left(SlashPos);
            UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] First path segment extracted: %s"), *FirstPathSegment);
        }
    }
    
    // 如果未找到常规路径，尝试通过第一段目录名查找插件
    if (!FirstPathSegment.IsEmpty())
    {
        UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] Trying to locate plugin by first path segment: %s"), *FirstPathSegment);
        
        // 使用插件管理器直接获取插件信息
        TArray<TSharedRef<IPlugin>> AllPlugins = IPluginManager::Get().GetDiscoveredPlugins();
        bool bFoundPlugin = false;
        TSharedPtr<IPlugin> MatchedPlugin;
        
        // 查找匹配的插件
        for (const TSharedRef<IPlugin>& PluginRef : AllPlugins)
        {
            FString PluginName = PluginRef->GetName();
            
            if (PluginName.Equals(FirstPathSegment, ESearchCase::IgnoreCase))
            {
                bFoundPlugin = true;
                MatchedPlugin = PluginRef;
                FString PluginBaseDir = PluginRef->GetBaseDir();
                UE_LOG(LogUnLua, Display, TEXT("[UnLua] Found matching plugin: %s, directory: %s"), *PluginName, *PluginBaseDir);
                
                FString PluginScriptDir = FPaths::Combine(PluginBaseDir, TEXT("Content/Script"));
                
                // 设置可能的Lua文件路径
                FString PossibleFileName1 = FPaths::Combine(PluginScriptDir, *RelativePath) + TEXT(".lua");
                FString PossibleFileName2 = FPaths::Combine(PluginScriptDir, *TemplateName) + TEXT(".lua");
                
                PossibleFileNames.Add(PossibleFileName1);
                PossibleFileNames.Add(PossibleFileName2);
                break;
            }
        }
        
        // 如果未通过插件管理器找到，则回退到文件系统查找
        if (!bFoundPlugin)
        {
            // 首先检查是否是GameFeatures插件
            FString GameFeaturesDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/GameFeatures"));
            FString PotentialGameFeaturePlugin = FPaths::Combine(GameFeaturesDir, FirstPathSegment);
            
            if (FPaths::DirectoryExists(PotentialGameFeaturePlugin))
            {
                UE_LOG(LogUnLua, Display, TEXT("[UnLua] Found matching GameFeature plugin: %s"), *PotentialGameFeaturePlugin);
                
                FString PluginScriptDir = FPaths::Combine(PotentialGameFeaturePlugin, TEXT("Content/Script"));
                
                // 设置可能的Lua文件路径
                FString PossibleFileName1 = FPaths::Combine(PluginScriptDir, *RelativePath) + TEXT(".lua");
                FString PossibleFileName2 = FPaths::Combine(PluginScriptDir, *TemplateName) + TEXT(".lua");
                
                PossibleFileNames.Add(PossibleFileName1);
                PossibleFileNames.Add(PossibleFileName2);
    }
    else
            {
                // 检查是否是普通插件
                FString PluginsRootDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins"));
                FString PotentialPlugin = FPaths::Combine(PluginsRootDir, FirstPathSegment);
                
                if (FPaths::DirectoryExists(PotentialPlugin))
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] Found matching regular plugin: %s"), *PotentialPlugin);
                    
                    FString PluginScriptDir = FPaths::Combine(PotentialPlugin, TEXT("Content/Script"));
                    
                    // 设置可能的Lua文件路径
                    FString PossibleFileName1 = FPaths::Combine(PluginScriptDir, *RelativePath) + TEXT(".lua");
                    FString PossibleFileName2 = FPaths::Combine(PluginScriptDir, *TemplateName) + TEXT(".lua");
                    
                    PossibleFileNames.Add(PossibleFileName1);
                    PossibleFileNames.Add(PossibleFileName2);
                }
            }
        }
    }
    
    // 尝试所有可能的文件位置
    bool bFoundFile = false;
    for (const FString& PossibleFile : PossibleFileNames)
    {
        if (IFileManager::Get().FileExists(*PossibleFile))
        {
            UE_LOG(LogUnLua, Display, TEXT("[UnLua] Found Lua file at: %s"), *PossibleFile);
            FPlatformProcess::ExploreFolder(*PossibleFile);
            bFoundFile = true;
            break;
        }
    }
    
    // 如果按标准路径没找到文件，尝试使用插件管理器再次查找
    if (!bFoundFile && !FirstPathSegment.IsEmpty())
    {
        // 获取所有插件信息并遍历
        TArray<TSharedRef<IPlugin>> AllPlugins = IPluginManager::Get().GetDiscoveredPlugins();
        
        // 尝试以插件名为关键字搜索
        for (const TSharedRef<IPlugin>& PluginRef : AllPlugins)
        {
            FString PluginName = PluginRef->GetName();
            if (PluginName.Equals(FirstPathSegment, ESearchCase::IgnoreCase))
            {
                // 获取插件的真实物理路径
                FString PluginBaseDir = PluginRef->GetBaseDir();
                FString PluginScriptDir = FPaths::Combine(PluginBaseDir, TEXT("Content/Script"));
                
                // 检查修正后的两个可能的文件路径
                FString AdjustedFileName1 = FPaths::Combine(PluginScriptDir, TEXT("TEST.lua"));
                
                // 使用中间变量来避免指针加法
                FString TemplateFileName = TemplateName + TEXT(".lua");  
                FString AdjustedFileName2 = FPaths::Combine(PluginScriptDir, TemplateFileName);
                
                UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 使用插件实际路径尝试: %s"), *AdjustedFileName1);
                if (IFileManager::Get().FileExists(*AdjustedFileName1))
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 找到文件(使用TEST名称): %s"), *AdjustedFileName1);
                    FPlatformProcess::ExploreFolder(*AdjustedFileName1);
                    bFoundFile = true;
                    break;
                }
                
                UE_LOG(LogUnLua, Verbose, TEXT("[UnLua] 使用插件实际路径尝试: %s"), *AdjustedFileName2);
                if (IFileManager::Get().FileExists(*AdjustedFileName2))
                {
                    UE_LOG(LogUnLua, Display, TEXT("[UnLua] 找到文件(使用模板名称): %s"), *AdjustedFileName2);
                    FPlatformProcess::ExploreFolder(*AdjustedFileName2);
                    bFoundFile = true;
                    break;
                }
            }
        }
    }
    
    if (!bFoundFile)
    {
        UE_LOG(LogUnLua, Warning, TEXT("[UnLua] Could not find Lua file in any location"));
        FNotificationInfo NotificationInfo(FText::FromString("UnLua Notification"));
        NotificationInfo.Text = LOCTEXT("FileNotExist", "The file does not exist.");
        NotificationInfo.bFireAndForget = true;
        NotificationInfo.ExpireDuration = 5.0f;
        NotificationInfo.bUseThrobber = false;
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

#undef LOCTEXT_NAMESPACE

