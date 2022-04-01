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
#include "LuaDelegateHandler.h"
#include "ReflectionUtils/FunctionDesc.h"

namespace UnLua
{
    class FDelegateRegistry
    {
    public:
        explicit FDelegateRegistry(lua_State* GL);

        lua_State* GL;

        void Bind(lua_State* L, int32 Index, FScriptDelegate* Delegate, UObject* Lifecycle);

        void Register(FScriptDelegate* ScriptDelegate, FDelegateProperty* DelegateProperty);

        void Execute(const ULuaDelegateHandler* Handler, void* Params, int32 LuaRef);

        int32 Execute(lua_State* L, FScriptDelegate* Delegate, int32 NumParams, int32 FirstParamIndex);

        void Unbind(FScriptDelegate* Delegate);

    private:
        FFunctionDesc* GetSignatureDesc(const FScriptDelegate* Delegate);

        struct FDelegateInfo
        {
            FDelegateProperty* Property;
            FFunctionDesc* Desc;
        };

        TMap<FScriptDelegate*, FDelegateInfo> Delegates;
    };
}
