// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
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

#include "LuaModuleLocator.h"
#include "UnLuaInterface.h"

FString ULuaModuleLocator::Locate(const UObject* Object)
{
    const UObject* CDO;
    if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        CDO = Object;
    }
    else
    {
        const auto Class = Cast<UClass>(Object);
        CDO = Class ? Class->GetDefaultObject() : Object->GetClass()->GetDefaultObject();
    }

    if (CDO->HasAnyFlags(RF_NeedInitialization))
    {
        // CDO还没有初始化完成
        return "";
    }

    if (!CDO->GetClass()->ImplementsInterface(UUnLuaInterface::StaticClass()))
    {
        return "";
    }

    return IUnLuaInterface::Execute_GetModuleName(CDO);
}

FString ULuaModuleLocator_ByPackage::Locate(const UObject* Object)
{
    const auto Class = Object->IsA<UClass>() ? static_cast<const UClass*>(Object) : Object->GetClass();
    const auto Key = Class->GetFName();
    const auto Cached = Cache.Find(Key);
    if (Cached)
        return *Cached;

    FString ModuleName;
    if (Class->IsNative())
    {
        ModuleName = Class->GetName();
    }
    else
    {
        FString OuterName = Object->GetOutermost()->GetName();
        
        // 检查是否来自GameFeature插件
        const bool bFromGameFeature = OuterName.Contains(TEXT("/GameFeatures/"));
        const bool bFromPlugin = OuterName.Contains(TEXT("/Plugins/"));
        
        if (bFromGameFeature || bFromPlugin)
        {
            // 提取插件名称
            FString PluginName;
            int32 PluginPathPos = INDEX_NONE;
            
            if (bFromGameFeature)
            {
                // 处理 GameFeature 插件
                const int32 GameFeaturesPos = OuterName.Find(TEXT("/GameFeatures/"), ESearchCase::IgnoreCase);
                if (GameFeaturesPos != INDEX_NONE)
                {
                    const int32 StartPos = GameFeaturesPos + FString(TEXT("/GameFeatures/")).Len();
                    const int32 EndPos = OuterName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartPos);
                    if (EndPos != INDEX_NONE)
                    {
                        PluginName = OuterName.Mid(StartPos, EndPos - StartPos);
                        PluginPathPos = GameFeaturesPos;
                    }
                }
            }
            else if (bFromPlugin)
            {
                // 处理普通插件
                const int32 PluginsPos = OuterName.Find(TEXT("/Plugins/"), ESearchCase::IgnoreCase);
                if (PluginsPos != INDEX_NONE)
                {
                    const int32 StartPos = PluginsPos + FString(TEXT("/Plugins/")).Len();
                    const int32 EndPos = OuterName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartPos);
                    if (EndPos != INDEX_NONE)
                    {
                        PluginName = OuterName.Mid(StartPos, EndPos - StartPos);
                        PluginPathPos = PluginsPos;
                    }
                }
            }
            
            // 处理插件中的蓝图
            if (!PluginName.IsEmpty() && PluginPathPos != INDEX_NONE)
            {
                // 获取资源路径部分
                FString ResourcePath;
                const int32 ContentPos = OuterName.Find(TEXT("/Content/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PluginPathPos);
                if (ContentPos != INDEX_NONE)
                {
                    const int32 ScriptPartStart = ContentPos + FString(TEXT("/Content/")).Len();
                    ResourcePath = OuterName.Mid(ScriptPartStart);
                    
                    // 获取类名部分（最后一个斜杠后的部分）
                    FString ClassName;
                    const int32 LastSlashPos = ResourcePath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
                    if (LastSlashPos != INDEX_NONE)
                    {
                        ClassName = ResourcePath.Mid(LastSlashPos + 1);
                        
                        // 构建最终的模块名
                        // 对于GameFeature插件，使用: 插件名.类名
                        // 例如: AstraAI.BP_TEST_C
                        ModuleName = PluginName + TEXT(".") + ClassName;
                    }
                    else
                    {
                        ModuleName = PluginName + TEXT(".") + ResourcePath;
                    }
                }
                else
                {
                    // 如果找不到Content目录，使用类名
                    ModuleName = PluginName + TEXT(".") + Class->GetName();
                }
            }
            else
            {
                // 使用默认处理
                const auto ChopCount = OuterName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1) + 1;
                ModuleName = OuterName.Replace(TEXT("/"), TEXT(".")).RightChop(ChopCount);
            }
        }
        else
        {
            // 普通资产的处理方式（不变）
            const auto ChopCount = OuterName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1) + 1;
            ModuleName = OuterName.Replace(TEXT("/"), TEXT(".")).RightChop(ChopCount);
        }
    }
    
    Cache.Add(Key, ModuleName);
    return ModuleName;
}
