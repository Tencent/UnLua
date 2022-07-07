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

#include "UObject/Script.h"
#include "LuaFunction.h"
#include "ReflectionUtils/FunctionDesc.h"

namespace UnLua
{
    class FLuaEnv;

    class FFunctionRegistry
    {
    public:
        explicit FFunctionRegistry(FLuaEnv* Env);

        void NotifyUObjectDeleted(UObject* Object);
        
        void Invoke(ULuaFunction* Function, UObject* Context, FFrame& Stack, RESULT_DECL);

    private:
        struct FFunctionInfo
        {
            lua_Integer LuaRef;
            TUniquePtr<FFunctionDesc> Desc;
        };

        FLuaEnv* Env;
        TMap<ULuaFunction*, FFunctionInfo> LuaFunctions;
    };
}
