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

static const FName NAME_Dummy = TEXT("Dummy");

void ULuaDelegateHandler::Dummy()
{
}

bool ULuaDelegateHandler::IsAlive() const
{
    return Lifecycle.IsValid();
}

void ULuaDelegateHandler::BindTo(FDelegateProperty* InProperty, FScriptDelegate* InDelegate)
{
    Property = InProperty;
    Delegate = InDelegate;

    InDelegate->BindUFunction(this, NAME_Dummy);
}

void ULuaDelegateHandler::AddTo(FMulticastDelegateProperty* InProperty, void* InDelegate)
{
    Property = InProperty;
    Delegate = InDelegate;

    FScriptDelegate DynamicDelegate;
    DynamicDelegate.BindUFunction(this, NAME_Dummy);
    TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(InProperty, MoveTemp(DynamicDelegate), nullptr, InDelegate);
}

void ULuaDelegateHandler::RemoveFrom(FMulticastDelegateProperty* InProperty, void* InDelegate)
{
    FScriptDelegate DynamicDelegate;
    DynamicDelegate.BindUFunction(this, NAME_Dummy);
    TMulticastDelegateTraits<FMulticastDelegateType>::RemoveDelegate(InProperty, MoveTemp(DynamicDelegate), nullptr, InDelegate);
}

void ULuaDelegateHandler::ProcessEvent(UFunction* Function, void* Parms)
{
    Env->GetDelegateRegistry()->Execute(this, Parms);
}

ULuaDelegateHandler* ULuaDelegateHandler::CreateFrom(UnLua::FLuaEnv* InEnv, int32 InLuaRef, UObject* InLifecycle)
{
    const auto Ret = NewObject<ULuaDelegateHandler>(InLifecycle);
    Ret->Env = InEnv;
    Ret->LuaRef = InLuaRef;
    Ret->Lifecycle = InLifecycle;
    return Ret;
}
