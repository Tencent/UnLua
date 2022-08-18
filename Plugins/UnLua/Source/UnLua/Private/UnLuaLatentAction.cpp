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
#include "Engine/LatentActionManager.h"
#include "UnLuaLatentAction.h"
#include "LuaCore.h"
#include "UnLuaEx.h"

void UUnLuaLatentAction::OnCompleted(int32 InLinkage) const
{
    if (Callback.IsBound())
        Callback.Execute(InLinkage);
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
    Env->ResumeThread(InLinkage);
}

static int32 UUnLuaLatentAction_CreateInfo(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);

    if (NumParams <= 0)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UUnLuaLatentAction* Self = Cast<UUnLuaLatentAction>(UnLua::GetUObject(L, 1));
    if (!Self)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid source class!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 Linkage;
    if (NumParams <= 1)
    {
        const auto Env = UnLua::FLuaEnv::FindEnv(L);
        if (!Env)
        {
            UE_LOG(LogUnLua, Log, TEXT("%s: invalid L!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }
        Linkage = Env->FindOrAddThread(L);
        if (Linkage == LUA_REFNIL)
        {
            luaL_error(L, "coroutine thread required");
            return 0;
        }
        Self->Env = Env;
        Self->Callback.BindDynamic(Self, &UUnLuaLatentAction::OnLegacyCallback);
    }
    else
    {
        Linkage = lua_tointeger(L, 2);
    }

    const auto UserData = NewUserdataWithPadding(L, sizeof(FLatentActionInfo), "FLatentActionInfo");
    new(UserData) FLatentActionInfo(Linkage, GetTypeHash(FGuid::NewGuid()), TEXT("OnCompleted"), Self);
    return 1;
}

static const luaL_Reg Lib[] =
{
    {"CreateInfo", UUnLuaLatentAction_CreateInfo},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(UUnLuaLatentAction)
    ADD_LIB(Lib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(UUnLuaLatentAction)
