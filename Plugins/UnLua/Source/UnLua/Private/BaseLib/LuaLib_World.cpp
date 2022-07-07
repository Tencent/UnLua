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
 * for example:
 * World:SpawnActor(
 *  WeaponClass, InitialTransform, ESpawnActorCollisionHandlingMethod.AlwaysSpawn,
 *  OwnerActor, Instigator, "Weapon.AK47_C", WeaponColor, ULevel, Name
 * )
 * the last four parameters "Weapon.AK47_C", 'WeaponColor', ULevel and Name are optional.
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

    if (NumParams > 8) {
        ULevel *Level = Cast<ULevel>(UnLua::GetUObject(L, 9));
        if (Level) {
            SpawnParameters.OverrideLevel = Level;
        }
    }

    if (NumParams > 9) {
        SpawnParameters.Name = FName(lua_tostring(L, 10));
    }

    {
        const char *ModuleName = NumParams > 6 ? lua_tostring(L, 7) : nullptr;
        int32 TableRef = LUA_NOREF;
        if (NumParams > 7 && lua_type(L, 8) == LUA_TTABLE)
        {
            lua_pushvalue(L, 8);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
        AActor *NewActor = World->SpawnActor(Class, &Transform, SpawnParameters);
        UnLua::PushUObject(L, NewActor);
    }

    return 1;
}

/**
 * World:SpawnActorEx(
 *  WeaponClass, InitialTransform, WeaponColor, "Weapon.AK47_C", ActorSpawnParameters
 * )
 */
static int32 UWorld_SpawnActorEx(lua_State *L)
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

    {
        int32 TableRef = LUA_NOREF;
        if (NumParams > 3 && lua_type(L, 4) == LUA_TTABLE)
        {
            lua_pushvalue(L, 4);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        const char *ModuleName = NumParams > 4 ? lua_tostring(L, 5) : nullptr;
        FActorSpawnParameters SpawnParameters;
        if (NumParams > 5)
        {
            FActorSpawnParameters *ActorSpawnParametersPtr = (FActorSpawnParameters*)GetCppInstanceFast(L, 6);
            if (ActorSpawnParametersPtr) {
                SpawnParameters = *ActorSpawnParametersPtr;
            }
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
        AActor *NewActor = World->SpawnActor(Class, &Transform, SpawnParameters);
        UnLua::PushUObject(L, NewActor);
    }

    return 1;
}

DEFINE_TYPE(ESpawnActorCollisionHandlingMethod)
DEFINE_TYPE(EObjectFlags)
DEFINE_TYPE(FActorSpawnParameters::ESpawnActorNameMode)

BEGIN_EXPORT_CLASS(FActorSpawnParameters)
    ADD_PROPERTY(Name)
    ADD_PROPERTY(Template)
    ADD_PROPERTY(Owner)
    ADD_PROPERTY(Instigator)
    ADD_PROPERTY(OverrideLevel)
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    ADD_PROPERTY(OverridePackage)
    ADD_PROPERTY(OverrideParentComponent)
    ADD_PROPERTY(OverrideActorGuid)
#endif
#endif
    ADD_PROPERTY(SpawnCollisionHandlingOverride)
    ADD_FUNCTION(IsRemoteOwned)
    ADD_BITFIELD_BOOL_PROPERTY(bNoFail)
    ADD_BITFIELD_BOOL_PROPERTY(bDeferConstruction)
    ADD_BITFIELD_BOOL_PROPERTY(bAllowDuringConstructionScript)
#if WITH_EDITOR
    ADD_BITFIELD_BOOL_PROPERTY(bTemporaryEditorActor)
    ADD_BITFIELD_BOOL_PROPERTY(bHideFromSceneOutliner)
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    ADD_BITFIELD_BOOL_PROPERTY(bCreateActorPackage)
#endif
    ADD_PROPERTY(NameMode)
    ADD_PROPERTY(ObjectFlags)
#endif
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FActorSpawnParameters)

static const luaL_Reg UWorldLib[] =
{
    { "SpawnActor", UWorld_SpawnActor },
    { "SpawnActorEx", UWorld_SpawnActorEx },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(UWorld)
    ADD_LIB(UWorldLib)
    ADD_FUNCTION(GetTimeSeconds)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(UWorld)
