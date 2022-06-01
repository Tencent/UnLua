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

#include "Engine/World.h"
#include "LuaDelegateHandler.h"
#include "LuaEnv.h"

static const FName NAME_Dummy = TEXT("Dummy");

ULuaDelegateHandler::ULuaDelegateHandler()
{
    LuaRef = LUA_NOREF;
    Env.Reset();
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

void ULuaDelegateHandler::BeginDestroy()
{
    if (Env.IsValid())
    {
        const auto PinnedEnv = Env.Pin();
        PinnedEnv->GetObjectRegistry()->NotifyUObjectLuaGC(this);
    }
    UObject::BeginDestroy();
}

void ULuaDelegateHandler::ProcessEvent(UFunction* Function, void* Parms)
{
    if (!Env.IsValid())
        return;
    Env.Pin()->GetDelegateRegistry()->Execute(this, Parms);
}

ULuaDelegateHandler* ULuaDelegateHandler::CreateFrom(UnLua::FLuaEnv* InEnv, int32 InLuaRef, UObject* InOwner, UObject* InSelfObject)
{
    UObject* Outer;

    if (InSelfObject)
        Outer = InSelfObject->GetOuter();
    else if (InOwner)
        Outer = InOwner->GetOuter();
    else
        Outer = nullptr;

    if (!Outer)
        Outer = GetTransientPackage();

    const auto Ret = NewObject<ULuaDelegateHandler>(Outer);
    Ret->Env = InEnv->AsShared();
    Ret->LuaRef = InLuaRef;
    Ret->Owner = InOwner;
    Ret->SelfObject = InSelfObject;
    return Ret;
}
