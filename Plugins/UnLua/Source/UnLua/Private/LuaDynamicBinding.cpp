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

#include "LuaDynamicBinding.h"
#include "lua.hpp"

FLuaDynamicBinding GLuaDynamicBinding;

bool FLuaDynamicBinding::IsValid(UClass *InClass) const
{
    //return Class && InClass->IsChildOf(Class) && ModuleName.Len() > 0;
    return Class && Class == InClass && ModuleName.Len() > 0;
}

bool FLuaDynamicBinding::Setup(UClass *InClass, const TCHAR *InModuleName, int32 InInitializerTableRef)
{
    if (!InClass || (Class && Class != InClass) || (ModuleName.Len() > 0 && ModuleName != InModuleName) || (!InModuleName && InInitializerTableRef == INDEX_NONE))
    {
        return false;
    }
    Class = InClass;
    ModuleName = InModuleName;
    InitializerTableRef = InInitializerTableRef;
    return true;
}

int32 FLuaDynamicBinding::Cleanup()
{
    Class = nullptr;
    ModuleName.Empty();
    int32 TableRef = InitializerTableRef;
    InitializerTableRef = INDEX_NONE;
    return TableRef;
}

FScopedLuaDynamicBinding::FScopedLuaDynamicBinding(lua_State *InL, UClass *Class, const TCHAR *ModuleName, int32 InitializerTableRef)
    : L(InL), bValid(false)
{
    if (L)
    {
        bValid = GLuaDynamicBinding.Setup(Class, ModuleName, InitializerTableRef);
    }
}

FScopedLuaDynamicBinding::~FScopedLuaDynamicBinding()
{
    if (bValid)
    {
        int32 InitializerTableRef = GLuaDynamicBinding.Cleanup();
        if (InitializerTableRef != INDEX_NONE)
        {
            check(L);
            luaL_unref(L, LUA_REGISTRYINDEX, InitializerTableRef);
        }
    }
}
