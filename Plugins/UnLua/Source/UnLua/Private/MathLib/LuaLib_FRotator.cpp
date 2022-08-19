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

#include "UnLuaCompatibility.h"
#include "UnLuaEx.h"
#include "LuaLib_Math.h"

static int32 FRotator_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FRotator);
    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FRotator(ForceInitToZero);
            break;
        }
    case 2:
        {
            const float& F = lua_tonumber(L, 2);
            new(Userdata) FRotator(F);
            break;
        }
    case 4:
        {
            const unluaReal& Pitch = lua_tonumber(L, 2);
            const unluaReal& Yaw = lua_tonumber(L, 3);
            const unluaReal& Roll = lua_tonumber(L, 4);
            new(Userdata) FRotator(Pitch, Yaw, Roll);
            break;
        }
    default:
        {
            return luaL_error(L, "invalid parameters");
        }
    }

    return 1;
}

static int32 GetRotatorScaledAxis(lua_State* L, EAxis::Type Axis, int32 MinParams)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < MinParams)
        return luaL_error(L, "invalid parameters");

    FRotator* A = (FRotator*)GetCppInstanceFast(L, 1);
    if (!A)
        return luaL_error(L, "invalid FRotator");

    if (NumParams > MinParams)
    {
        const int32 OutParamIndex = MinParams + 1;
        FVector* B = (FVector*)GetCppInstanceFast(L, OutParamIndex);
        if (B)
        {
            *B = FRotationMatrix(*A).GetScaledAxis(Axis);
            lua_pushvalue(L, OutParamIndex);
            return 1;
        }
    }

    void* Userdata = NewTypedUserdata(L, FVector);
    new(Userdata) FVector(FRotationMatrix(*A).GetScaledAxis(Axis));
    return 1;
};

static int32 FRotator_GetRightVector(lua_State* L)
{
    return GetRotatorScaledAxis(L, EAxis::Y, 1);
}

static int32 FRotator_GetUpVector(lua_State* L)
{
    return GetRotatorScaledAxis(L, EAxis::Z, 1);
}

static int32 FRotator_GetUnitAxis(lua_State* L)
{
    return GetRotatorScaledAxis(L, (EAxis::Type)lua_tointeger(L, 2), 2);
}

static int32 FRotator_Set(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
        return luaL_error(L, "invalid parameters");

    FRotator* V = (FRotator*)GetCppInstanceFast(L, 1);
    if (!V)
        return luaL_error(L, "invalid FRotator");

    UnLua::TFieldSetter3<unluaReal>::Set(L, NumParams, &V->Pitch);
    return 0;
}

static const luaL_Reg FRotatorLib[] =
{
    {"GetRightVector", FRotator_GetRightVector},
    {"GetUpVector", FRotator_GetUpVector},
    {"GetUnitAxis", FRotator_GetUnitAxis},
    {"__tostring", UnLua::TMathUtils<FRotator>::ToString},
    {"Set", FRotator_Set},
    {"__call", FRotator_New},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(FRotator)
    ADD_FUNCTION(Normalize)
    ADD_FUNCTION(GetNormalized)
    ADD_FUNCTION(RotateVector)
    ADD_FUNCTION(UnrotateVector)
    ADD_FUNCTION(Clamp)
    ADD_NAMED_FUNCTION("GetForwardVector", Vector)
    ADD_NAMED_FUNCTION("ToVector", Vector)
    ADD_NAMED_FUNCTION("ToEuler", Euler)
    ADD_NAMED_FUNCTION("ToQuat", Quaternion)
    ADD_NAMED_FUNCTION("Inverse", GetInverse)
    ADD_CONST_FUNCTION_EX("__add", FRotator, operator+, const FRotator&)
    ADD_CONST_FUNCTION_EX("__sub", FRotator, operator-, const FRotator&)
    ADD_CONST_FUNCTION_EX("__mul", FRotator, operator*, float)
    ADD_FUNCTION_EX("Add", FRotator, operator+=, const FRotator&)
    ADD_FUNCTION_EX("Sub", FRotator, operator-=, const FRotator&)
    ADD_FUNCTION_EX("Mul", FRotator, operator*=, unluaReal)
    ADD_LIB(FRotatorLib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FRotator)
