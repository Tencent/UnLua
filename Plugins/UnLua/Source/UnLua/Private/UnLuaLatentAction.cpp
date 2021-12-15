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

#include "UnLuaLatentAction.h"

#include "LuaContext.h"

void UUnLuaLatentAction::OnCompleted(int32 InLinkage) const
{
    if (Callback.IsBound())
        Callback.Execute(InLinkage);
}

FLatentActionInfo UUnLuaLatentAction::CreateInfo(const int32 Linkage)
{
    return FLatentActionInfo(Linkage, GetTypeHash(FGuid::NewGuid()), TEXT("OnCompleted"), this);
}

FLatentActionInfo UUnLuaLatentAction::CreateInfoForLegacy()
{
    Callback.BindDynamic(this, &UUnLuaLatentAction::OnLegacyCallback);
    return CreateInfo(MAGIC_LEGACY_LINKAGE);
}

bool UUnLuaLatentAction::GetTickableWhenPaused()
{
    return bTickEvenWhenPaused;
}

void UUnLuaLatentAction::SetTickableWhenPaused(bool bTickableWhenPaused)
{
    bTickEvenWhenPaused = bTickableWhenPaused;
}

void UUnLuaLatentAction::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (World)
    {
        GetClass()->ClassFlags |= CLASS_CompiledFromBlueprint;
        World->GetLatentActionManager().ProcessLatentActions(this, DeltaTime);
        GetClass()->ClassFlags &= ~CLASS_CompiledFromBlueprint;
    }
}

bool UUnLuaLatentAction::IsTickable() const
{
    return bTickEvenWhenPaused;
}

TStatId UUnLuaLatentAction::GetStatId() const
{
    return GetStatID();
}

void UUnLuaLatentAction::OnLegacyCallback(int32 InLinkage)
{
    Callback.Unbind();
    GLuaCxt->ResumeThread(InLinkage);
}
