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


#include "LuaDelegateHandler.h"
#include "LuaEnv.h"

void ULuaDelegateHandler::Dummy()
{
}

bool ULuaDelegateHandler::IsAlive() const
{
    return Lifecycle.IsValid();
}

void ULuaDelegateHandler::BindTo(FScriptDelegate* InDelegate, UnLua::FLuaEnv* InEnv, int32 InLuaRef, UObject* InLifeCycle)
{
    static const FName NAME_Invoke = TEXT("Dummy");

    Env = InEnv;
    LuaRef = InLuaRef;
    Lifecycle = InLifeCycle;
    Delegate = InDelegate;
    Delegate->BindUFunction(this, NAME_Invoke);
}

void ULuaDelegateHandler::ProcessEvent(UFunction* Function, void* Parms)
{
    Env->GetDelegateRegistry()->Execute(this, Parms, LuaRef);
}
