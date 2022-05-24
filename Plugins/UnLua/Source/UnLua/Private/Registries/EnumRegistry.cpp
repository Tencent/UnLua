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

#include "CollisionHelper.h"
#include "EnumRegistry.h"
#include "LowLevel.h"
#include "LuaCore.h"
#include "LuaEnv.h"

namespace UnLua
{
    TMap<UEnum*, FEnumDesc*> FEnumRegistry::Enums;
    TMap<FName, FEnumDesc*> FEnumRegistry::Name2Enums;

    FEnumRegistry::FEnumRegistry(FLuaEnv* Env)
        : Env(Env)
    {
        const auto L = Env->GetMainState();
        FCollisionHelper::Initialize();
        RegisterECollisionChannel(L);
        RegisterEObjectTypeQuery(L);
        RegisterETraceTypeQuery(L);
    }

    FEnumDesc* FEnumRegistry::Find(const char* InName)
    {
        const FName Key = InName;
        FEnumDesc** Ret = Name2Enums.Find(Key);
        return Ret ? *Ret : nullptr;
    }

    FEnumDesc* FEnumRegistry::StaticRegister(const char* MetatableName)
    {
        FEnumDesc* Ret = Find(MetatableName);
        if (Ret)
            return Ret;

        const FString EnumName = UTF8_TO_TCHAR(MetatableName);
        UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, *EnumName);
        if (!Enum)
        {
            Enum = LoadObject<UEnum>(nullptr, *EnumName);
            if (!Enum)
                return nullptr;
        }

        Ret = new FEnumDesc(Enum);
        Enums.Add(Enum, Ret);
        Name2Enums.Add(MetatableName, Ret);
        return Ret;
    }

    bool FEnumRegistry::StaticUnregister(const UObjectBase* Enum)
    {
        FEnumDesc* EnumDesc;
        if (!Enums.RemoveAndCopyValue((UEnum*)Enum, EnumDesc))
            return false;
        EnumDesc->UnLoad();
        return true;
    }

    void FEnumRegistry::Cleanup()
    {
        for (const auto Pair : Name2Enums)
            delete Pair.Value;
        Name2Enums.Empty();
        Enums.Empty();
    }

    FEnumDesc* FEnumRegistry::Register(const char* MetatableName)
    {
        FEnumDesc* EnumDesc = StaticRegister(MetatableName);
        if (!EnumDesc)
            return nullptr;

        const auto L = Env->GetMainState();
        if (luaL_getmetatable(L, MetatableName) == LUA_TTABLE)
        {
            lua_pop(L, 1);
            return EnumDesc;
        }
        lua_pop(L, 1);

        luaL_newmetatable(L, MetatableName);

        lua_pushstring(L, "__index");
        lua_pushcfunction(L, Enum_Index);
        lua_rawset(L, -3);

        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, Enum_Delete);
        lua_rawset(L, -3);

        // add other members here
        lua_pushstring(L, "GetMaxValue");
        lua_pushvalue(L, -2); // EnumTable
        lua_pushcclosure(L, Enum_GetMaxValue, 1); // closure
        lua_rawset(L, -3);

        lua_pushstring(L, "GetNameByValue");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, Enum_GetNameStringByValue, 1);
        lua_rawset(L, -3);

        lua_pushstring(L, "GetNameStringByValue");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, Enum_GetNameStringByValue, 1);
        lua_rawset(L, -3);

        lua_pushstring(L, "GetDisplayNameTextByValue");
        lua_pushvalue(L, -2);
        lua_pushcclosure(L, Enum_GetDisplayNameTextByValue, 1);
        lua_rawset(L, -3);

        lua_pushvalue(L, -1); // set metatable to self
        lua_setmetatable(L, -2);

        SetTableForClass(L, MetatableName);
        return EnumDesc;
    }

    FEnumDesc* FEnumRegistry::Register(const UEnum* Enum)
    {
        const auto MetatableName = LowLevel::GetMetatableName(Enum);
        return Register(TCHAR_TO_UTF8(*MetatableName));
    }
}
