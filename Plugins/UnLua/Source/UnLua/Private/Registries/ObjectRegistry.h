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
#include "lauxlib.h"
#include "lstate.h"
#include "UnLuaBase.h"

class FObjectRegistry
{
public:
    FObjectRegistry(lua_State* GL);

    void NotifyUObjectDeleted(UObject* Object);
    
    template <typename T>
    void Push(lua_State* L, TSharedPtr<T> Ptr);

    template <typename T>
    TSharedPtr<T> Get(lua_State* L, int Index);

    int Ref(lua_State* L, UObject* Object, const int Index);

    bool IsBound(UObject* Object);

private:
    TMap<UObject*, int32> ObjectRefs;
    lua_State* GL;
};

template <typename T>
void FObjectRegistry::Push(lua_State* L, TSharedPtr<T> Ptr)
{
    const auto Userdata = UnLua::NewSmartPointer(L, sizeof(TSharedPtr<T>), "TSharedPtr");
    new(Userdata) TSharedPtr<T>(Ptr);
}

template <typename T>
TSharedPtr<T> FObjectRegistry::Get(lua_State* L, int Index)
{
    const auto Ptr = UnLua::GetSmartPointer(L, Index);
    return *static_cast<TSharedPtr<T>*>(Ptr);
}
