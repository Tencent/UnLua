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
        ModuleName = Object->GetOutermost()->GetName();
        const auto ChopCount = ModuleName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1) + 1;
        ModuleName = ModuleName.Replace(TEXT("/"), TEXT(".")).RightChop(ChopCount);
    }
    Cache.Add(Key, ModuleName);
    return ModuleName;
}
