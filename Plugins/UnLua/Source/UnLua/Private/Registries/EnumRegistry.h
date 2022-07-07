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

#include "lua.h"
#include "ReflectionUtils/EnumDesc.h"

namespace UnLua
{
    class FLuaEnv;

    class FEnumRegistry
    {
    public:
        explicit FEnumRegistry(FLuaEnv* Env);

        static FEnumDesc* Find(const char* InName);

        static FEnumDesc* StaticRegister(const char* MetatableName);

        static bool StaticUnregister(const UObjectBase* Enum);

        static void Cleanup();

        FEnumDesc* Register(const char* MetatableName);

        FEnumDesc* Register(const UEnum* Enum);

    private:
        static TMap<UEnum*, FEnumDesc*> Enums;
        static TMap<FName, FEnumDesc*> Name2Enums;
        FLuaEnv* Env;
    };
}
