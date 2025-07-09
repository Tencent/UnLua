// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 Tencent. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "UnLuaEditorCore.h"
#include "UnLuaInterface.h"
#include "UnLuaPrivate.h"
#include "UnLuaSettings.h"
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"

ELuaBindingStatus GetBindingStatus(const UBlueprint* Blueprint)
{
    if (!Blueprint)
        return ELuaBindingStatus::NotBound;

    if (Blueprint->Status == EBlueprintStatus::BS_Dirty)
        return ELuaBindingStatus::Unknown;

    const auto Target = Blueprint->GeneratedClass;

    if (!IsValid(Target))
        return ELuaBindingStatus::NotBound;

    if (!Target->ImplementsInterface(UUnLuaInterface::StaticClass()))
        return ELuaBindingStatus::NotBound;

    const auto Settings = GetDefault<UUnLuaSettings>();
    if (!Settings || !Settings->ModuleLocatorClass)
        return ELuaBindingStatus::Unknown;

    const auto ModuleLocator = Cast<ULuaModuleLocator>(Settings->ModuleLocatorClass->GetDefaultObject());
    const auto ModuleName = ModuleLocator->Locate(Target);
    if (ModuleName.IsEmpty())
        return ELuaBindingStatus::Unknown;

    // 分解模块名称
    TArray<FString> ModuleNameParts;
    ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
    const auto TemplateName = ModuleNameParts.Last();
    
    // 转换为相对路径
    const auto RelativePath = ModuleName.Replace(TEXT("."), TEXT("/")) + TEXT(".lua");
    
    // 获取蓝图路径信息
    FString BlueprintPackagePath = Blueprint->GetOutermost()->GetName();
    FString BlueprintPath = Blueprint->GetPathName();
    FString AssetPath = Blueprint->GetPathName();
    FString OutermostName = Blueprint->GetOutermost()->GetName();
    FString OuterPathName = Blueprint->GetOutermost()->GetPathName();
    FString OutermostPath = Blueprint->GetOutermost()->GetPackage()->GetPathName();
    FString ClassPath = Blueprint->GeneratedClass->GetPathName();
    
    // 存储可能的Lua文件路径
    TArray<FString> PossiblePaths;
    
    // 检查默认路径
    const auto DefaultFullPath = GLuaSrcFullPath + "/" + RelativePath;
    PossiblePaths.Add(DefaultFullPath);
    
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
        }
    }
    
    // 合并所有可能的路径到一个数组，并检查每个路径是否包含插件关键词
    TArray<TPair<FString, FString>> AllPaths;
    AllPaths.Add(TPair<FString, FString>(BlueprintPackagePath, TEXT("BlueprintPackagePath")));
    AllPaths.Add(TPair<FString, FString>(OutermostPath, TEXT("OutermostPath")));
    AllPaths.Add(TPair<FString, FString>(OutermostName, TEXT("OutermostName")));
    AllPaths.Add(TPair<FString, FString>(OuterPathName, TEXT("OuterPathName")));
    AllPaths.Add(TPair<FString, FString>(ClassPath, TEXT("ClassPath")));
    AllPaths.Add(TPair<FString, FString>(AssetPath, TEXT("AssetPath")));
    AllPaths.Add(TPair<FString, FString>(BlueprintPath, TEXT("BlueprintPath")));
    
    // 遍历所有路径，检测插件类型
    FString PluginPathToUse;
    bool bIsGameFeaturePlugin = false;
    
    for (const auto& PathPair : AllPaths)
    {
        const FString& Path = PathPair.Key;
        
        // 检查是否包含GameFeatures路径
        if (Path.Contains(TEXT("/GameFeatures/")) || Path.Contains(TEXT("\\GameFeatures\\")))
        {
            PluginPathToUse = Path;
            bIsGameFeaturePlugin = true;
            break;
        }
        // 检查是否包含普通插件路径
        else if (Path.Contains(TEXT("/Plugins/")) || Path.Contains(TEXT("\\Plugins\\")))
        {
            PluginPathToUse = Path;
            break;
        }
    }
    
    // 如果还没找到插件路径，尝试通过第一段目录名查找匹配的插件
    if (PluginPathToUse.IsEmpty() && !FirstPathSegment.IsEmpty())
    {
        // 检查GameFeatures目录是否有匹配插件
        FString ProjectDir = FPaths::ProjectDir();
        FString GameFeaturesDir = FPaths::Combine(ProjectDir, TEXT("Plugins/GameFeatures"));
        FString PotentialGameFeaturePlugin = FPaths::Combine(GameFeaturesDir, FirstPathSegment);
        if (FPaths::DirectoryExists(PotentialGameFeaturePlugin))
        {
            bIsGameFeaturePlugin = true;
            // 构造一个包含插件路径的字符串，供后续处理使用
            PluginPathToUse = FString::Printf(TEXT("/GameFeatures/%s/%s"), *FirstPathSegment, *BlueprintPackagePath);
        }
        else
        {
            // 检查普通Plugins目录是否有匹配插件
            FString PluginsRootDir = FPaths::Combine(ProjectDir, TEXT("Plugins"));
            FString PotentialPlugin = FPaths::Combine(PluginsRootDir, FirstPathSegment);
            if (FPaths::DirectoryExists(PotentialPlugin))
            {
                // 构造一个包含插件路径的字符串，供后续处理使用
                PluginPathToUse = FString::Printf(TEXT("/Plugins/%s/%s"), *FirstPathSegment, *BlueprintPackagePath);
            }
        }
    }
    
    // 如果找到了插件路径
    if (!PluginPathToUse.IsEmpty())
    {
        FString PluginName;
        FString RemainingPath;
        
        // 如果通过目录名生成了路径，可以直接使用FirstPathSegment作为插件名
        if (!FirstPathSegment.IsEmpty() && PluginPathToUse.Contains(FirstPathSegment))
        {
            PluginName = FirstPathSegment;
        }
        else
        {
            // 处理不同的分隔符
            bool bUsesForwardSlash = true;
            FString SplitKeyword;
            
            if (bIsGameFeaturePlugin)
            {
                if (PluginPathToUse.Contains(TEXT("/GameFeatures/")))
                {
                    SplitKeyword = TEXT("/GameFeatures/");
                }
                else
                {
                    SplitKeyword = TEXT("\\GameFeatures\\");
                    bUsesForwardSlash = false;
                }
            }
            else
            {
                if (PluginPathToUse.Contains(TEXT("/Plugins/")))
                {
                    SplitKeyword = TEXT("/Plugins/");
                }
                else
                {
                    SplitKeyword = TEXT("\\Plugins\\");
                    bUsesForwardSlash = false;
                }
            }
            
            // 分离路径获取插件名
            PluginPathToUse.Split(SplitKeyword, nullptr, &RemainingPath);
            
            // 分隔插件名和剩余路径
            if (bUsesForwardSlash)
            {
                RemainingPath.Split(TEXT("/"), &PluginName, &RemainingPath);
            }
            else
            {
                RemainingPath.Split(TEXT("\\"), &PluginName, &RemainingPath);
            }
            
            // 如果提取失败，尝试更特殊的处理（从路径字符串中分析）
            if (PluginName.IsEmpty())
            {
                // 尝试用简单的字符串分析提取插件名
                int32 PluginsPos = PluginPathToUse.Find(SplitKeyword, ESearchCase::IgnoreCase, ESearchDir::FromStart);
                
                if (PluginsPos != INDEX_NONE)
                {
                    FString AfterPlugins = PluginPathToUse.Mid(PluginsPos + SplitKeyword.Len());
                    
                    // 提取第一个目录名作为插件名
                    int32 NextSlashPos = bUsesForwardSlash ? 
                        AfterPlugins.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart) : 
                        AfterPlugins.Find(TEXT("\\"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
                    
                    if (NextSlashPos != INDEX_NONE)
                    {
                        PluginName = AfterPlugins.Left(NextSlashPos);
                    }
                }
            }
        }
        
        if (!PluginName.IsEmpty())
        {
            // 构建插件的脚本路径
            FString PluginRoot;
            if (bIsGameFeaturePlugin)
            {
                PluginRoot = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/GameFeatures"), PluginName);
            }
            else
            {
                PluginRoot = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins"), PluginName);
            }
            
            FString PluginScriptDir = FPaths::Combine(PluginRoot, TEXT("Content/Script"));
            
            // 检查两种可能的文件路径
            FString PossiblePath1 = FPaths::Combine(PluginScriptDir, *RelativePath);
            FString PossiblePath2 = FPaths::Combine(PluginScriptDir, *TemplateName) + TEXT(".lua");
            
            PossiblePaths.Add(PossiblePath1);
            PossiblePaths.Add(PossiblePath2);
        }
    }
    
    // 尝试所有可能的路径
    for (const FString& Path : PossiblePaths)
    {
        if (FPaths::FileExists(Path))
            return ELuaBindingStatus::Bound;
    }
    
    // 如果所有位置都没找到，标记为无效绑定
    return ELuaBindingStatus::BoundButInvalid;
}
