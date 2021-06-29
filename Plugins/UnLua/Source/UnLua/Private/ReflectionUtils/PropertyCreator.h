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

#pragma once

#include "UnLuaBase.h"

class FPropertyDesc;

USTRUCT(noexport)
struct FPropertyCollector
{
};

class IPropertyCreator
{
public:
    static IPropertyCreator& Instance();

    virtual void Cleanup() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateBoolProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateIntProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateFloatProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateStringProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateNameProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateTextProperty() = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateEnumProperty(UEnum *Enum) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateClassProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateObjectProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateSoftClassProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateSoftObjectProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateWeakObjectProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateLazyObjectProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateInterfaceProperty(UClass *Class) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateStructProperty(UScriptStruct *Struct) = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> CreateProperty(FProperty *TemplateProperty) = 0;
};

#define GPropertyCreator IPropertyCreator::Instance()
