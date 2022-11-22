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

#include "lua.hpp"
#include "UnLuaBase.h"

USTRUCT(noexport)
struct FPropertyCollector
{
};

namespace UnLua
{
    class FLuaEnv;

    class FPropertyRegistry
    {
    public:
        explicit FPropertyRegistry(FLuaEnv* Env);

        void NotifyUObjectDeleted(UObject* Object);

        /**
         * Create a type interface according to Lua parameter's type
         */
        TSharedPtr<ITypeInterface> CreateTypeInterface(lua_State* L, int32 Index);

    private:
        TSharedPtr<ITypeInterface> GetBoolProperty();
        TSharedPtr<ITypeInterface> GetIntProperty();
        TSharedPtr<ITypeInterface> GetFloatProperty();
        TSharedPtr<ITypeInterface> GetStringProperty();
        TSharedPtr<ITypeInterface> GetNameProperty();
        TSharedPtr<ITypeInterface> GetTextProperty();
        TSharedPtr<ITypeInterface> GetFieldProperty(UField* Field);

        FLuaEnv* Env;
        UScriptStruct* PropertyCollector;
        TMap<UField*, TSharedPtr<ITypeInterface>> FieldProperties;
        TSharedPtr<ITypeInterface> BoolProperty;
        TSharedPtr<ITypeInterface> IntProperty;
        TSharedPtr<ITypeInterface> FloatProperty;
        TSharedPtr<ITypeInterface> StringProperty;
        TSharedPtr<ITypeInterface> NameProperty;
        TSharedPtr<ITypeInterface> TextProperty;
    };
}
