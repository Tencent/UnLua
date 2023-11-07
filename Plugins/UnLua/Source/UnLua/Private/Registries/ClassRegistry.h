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
#include "ReflectionUtils/ClassDesc.h"

namespace UnLua
{
    class FLuaEnv;

    class FClassRegistry
    {
    public:
        explicit FClassRegistry(FLuaEnv* Env);

        ~FClassRegistry();

        void Initialize();

        FClassDesc* Find(const char* TypeName);

        FClassDesc* Find(const UStruct* Type);

        FClassDesc* RegisterReflectedType(const char* MetatableName);

        FClassDesc* RegisterReflectedType(UStruct* Type);

        static UField* LoadReflectedType(const char* InName);

        void NotifyUObjectDeleted(UObject* Object);

        bool PushMetatable(lua_State* L, const char* MetatableName);

        bool TrySetMetatable(lua_State* L, const char* MetatableName);

        FClassDesc* Register(const char* MetatableName);

        FClassDesc* Register(const UStruct* Class);

        void Unregister(const UStruct* Class);

    private:
        FClassDesc* RegisterInternal(UStruct* Type, const FString& Name);

        void Unregister(const FClassDesc* ClassDesc, const bool bForce);

        TMap<UStruct*, FClassDesc*> Classes;
        TMap<FName, FClassDesc*> Name2Classes;

        FLuaEnv* Env;
    };
}
