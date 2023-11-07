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
#include "BaseLib/LuaLib_Enum.h"

namespace UnLua
{
    FEnumRegistry::FEnumRegistry(FLuaEnv* Env)
        : Env(Env)
    {
    }

    FEnumRegistry::~FEnumRegistry()
    {
        for (const auto Pair : Name2Enums)
            delete Pair.Value;
    }

    void FEnumRegistry::Initialize()
    {
        FCollisionHelper::Initialize();

        Register(StaticEnum<ECollisionChannel>(), &ECollisionChannel_Index);
        Register(StaticEnum<EObjectTypeQuery>(), &EObjectTypeQuery_Index);
        Register(StaticEnum<ETraceTypeQuery>(), &ETraceTypeQuery_Index);
    }

    FEnumDesc* FEnumRegistry::Find(const char* InName)
    {
        FEnumDesc** Ret = Name2Enums.Find(UTF8_TO_TCHAR(InName));
        return Ret ? *Ret : nullptr;
    }

    FEnumDesc* FEnumRegistry::Find(const UEnum* Enum)
    {
        FEnumDesc** Ret = Enums.Find(Enum);
        return Ret ? *Ret : nullptr;
    }

    void FEnumRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        Unregister((UEnum*)Object);
    }

    FEnumDesc* FEnumRegistry::Register(UEnum* Enum, lua_CFunction IndexFunc)
    {
        if (!IndexFunc)
            IndexFunc = &Enum_Index;

        const auto MetatableName = LowLevel::GetMetatableName(Enum);
        const auto Found = Name2Enums.Find(MetatableName);
        if (Found)
            return *Found;

        auto Ret = new FEnumDesc(Enum);
        Enums.Add(Enum, Ret);
        Name2Enums.Add(MetatableName, Ret);

        const auto L = Env->GetMainState();
        const auto MetatableNameUTF8 = FTCHARToUTF8(*MetatableName);
        if (luaL_getmetatable(L, MetatableNameUTF8.Get()) == LUA_TTABLE)
        {
            if (Ret->IsValid())
            {
                lua_pop(L, 1);
                return Ret;
            }
        }
        lua_pop(L, 1);

        luaL_newmetatable(L, MetatableNameUTF8.Get());

        lua_pushstring(L, "Object");
        Env->GetObjectRegistry()->Push(L, Ret->GetEnum());
        lua_rawset(L, -3);

        lua_pushstring(L, "__index");
        lua_pushcfunction(L, IndexFunc);
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

        SetTableForClass(L, MetatableNameUTF8.Get());
        return Ret;
    }

    void FEnumRegistry::Unregister(const UEnum* Enum)
    {
        const auto Desc = Find(Enum);
        if (!Desc)
            return;

        const auto L = Env->GetMainState();
        const auto MetatableName = Desc->GetName();
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, TCHAR_TO_UTF8(*MetatableName));

        Enums.Remove(Enum);
        Name2Enums.Remove(Desc->GetName());
        Desc->UnLoad();
    }
}
