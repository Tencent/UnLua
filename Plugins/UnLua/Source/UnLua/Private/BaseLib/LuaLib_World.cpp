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

#include "UnLuaEx.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "Engine/World.h"

/**
 * Spawn an actor. 
 * for example: World:SpawnActor(WeaponClass, InitialTransform, ESpawnActorCollisionHandlingMethod.AlwaysSpawn, Player, Player, "Weapon.AK47_C", WeaponColor), 
 * the last two parameters "Weapon.AK47_C" and 'WeaponColor' and optional.
 * see programming guide for detail.
 */
static int32 UWorld_SpawnActor(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        lua_pushnil(L);
        return 1;
    }

    UWorld *World = Cast<UWorld>(UnLua::GetUObject(L, 1));
    if (!World)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid world!"), ANSI_TO_TCHAR(__FUNCTION__));
        lua_pushnil(L);
        return 1;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (!Class)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid class!"), ANSI_TO_TCHAR(__FUNCTION__));
        lua_pushnil(L);
        return 1;
    }

    FTransform Transform;
    if (NumParams > 2)
    {
        FTransform *TransformPtr = (FTransform*)GetCppInstanceFast(L, 3);
        if (TransformPtr)
        {
            Transform = *TransformPtr;
        }
    }

    FActorSpawnParameters SpawnParameters;
    if (NumParams > 3)
    {
        uint8 CollisionHandlingOverride = (uint8)lua_tointeger(L, 4);
        SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)CollisionHandlingOverride;
    }
    if (NumParams > 4)
    {
        AActor *Owner = Cast<AActor>(UnLua::GetUObject(L, 5));
        check(!Owner || (Owner && World == Owner->GetWorld()));
        SpawnParameters.Owner = Owner;
    }
    if (NumParams > 5)
    {
        AActor *Actor = Cast<AActor>(UnLua::GetUObject(L, 6));
        if (Actor)
        {
            APawn *Instigator = Cast<APawn>(Actor);
            if (!Instigator)
            {
                Instigator = Actor->GetInstigator();
            }
            SpawnParameters.Instigator = Instigator;
        }
    }

    {
        const char *ModuleName = NumParams > 6 ? lua_tostring(L, 7) : nullptr;
        int32 TableRef = INDEX_NONE;
        if (NumParams > 7 && lua_type(L, 8) == LUA_TTABLE)
        {
            lua_pushvalue(L, 8);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, ANSI_TO_TCHAR(ModuleName), TableRef);
        AActor *NewActor = World->SpawnActor(Class, &Transform, SpawnParameters);
        UnLua::PushUObject(L, NewActor);
    }

    return 1;
}

static const luaL_Reg UWorldLib[] =
{
    { "SpawnActor", UWorld_SpawnActor },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(UWorld)
    ADD_LIB(UWorldLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(UWorld)
