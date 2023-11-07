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

ULuaDelegateHandler::ULuaDelegateHandler()
{
    LuaRef = LUA_NOREF;
    Registry = nullptr;
    Delegate = nullptr;
}

void ULuaDelegateHandler::Dummy()
{
}

void ULuaDelegateHandler::BindTo(FScriptDelegate* InDelegate)
{
    Delegate = InDelegate;
    InDelegate->BindUFunction(this, NAME_Dummy);
}

void ULuaDelegateHandler::AddTo(FMulticastDelegateProperty* InProperty, void* InDelegate)
{
    check(InDelegate);

    Delegate = InDelegate;
    FScriptDelegate DynamicDelegate;
    DynamicDelegate.BindUFunction(this, NAME_Dummy);
    TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(InProperty, MoveTemp(DynamicDelegate), nullptr, InDelegate);
}

UWorld* ULuaDelegateHandler::GetWorld() const
{
    if (SelfObject.IsValid())
        return SelfObject->GetWorld();
    return UObject::GetWorld();
}

void ULuaDelegateHandler::RemoveFrom(FMulticastDelegateProperty* InProperty, void* InDelegate)
{
    FScriptDelegate DynamicDelegate;
    DynamicDelegate.BindUFunction(this, NAME_Dummy);
    TMulticastDelegateTraits<FMulticastDelegateType>::RemoveDelegate(InProperty, MoveTemp(DynamicDelegate), nullptr, InDelegate);
}

void ULuaDelegateHandler::BeginDestroy()
{
    if (Registry)
        Registry->NotifyHandlerBeginDestroy(this);
    UObject::BeginDestroy();
}

void ULuaDelegateHandler::Reset()
{
    LuaRef = LUA_NOREF;
    Registry = nullptr;
    Delegate = nullptr;
}

void ULuaDelegateHandler::ProcessEvent(UFunction* Function, void* Parms)
{
    if (Registry)
        Registry->Execute(this, Parms);
}
