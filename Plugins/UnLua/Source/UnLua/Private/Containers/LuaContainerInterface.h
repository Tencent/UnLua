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

#include "LuaContext.h"

template <typename LuaContainerType>
class TLuaContainerInterface
{
public:
    virtual ~TLuaContainerInterface()
    {
        while (AllCachedContainer.Num() > 0)
        {
            AllCachedContainer.Last()->DetachInterface();
        }
    }

    void AddContainer(LuaContainerType *Container)
    {
        AllCachedContainer.Add(Container);
    }

    void RemoveContainer(LuaContainerType *Container)
    {
        if (Container && AllCachedContainer.Remove(Container) > 0)
        {
            RemoveCachedScriptContainer(*GLuaCxt, Container->GetContainerPtr());
        }
    }

    virtual TSharedPtr<UnLua::ITypeInterface> GetInnerInterface() const = 0;
    virtual TSharedPtr<UnLua::ITypeInterface> GetExtraInterface() const = 0;

protected:
    TArray<LuaContainerType*> AllCachedContainer;
};
