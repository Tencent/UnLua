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
#include "LuaLib_Math.h"

static int32 FQuat_New(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    void *Userdata = NewTypedUserdata(L, FQuat);
    FQuat *V = new(Userdata) FQuat(ForceInit);
    UnLua::TFieldSetter4<float>::Set(L, NumParams, &V->X);
    return 1;
}

static int32 FQuat_Normalize(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FQuat *V = (FQuat*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FQuat!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    V->Normalize();
    return 0;
}

/**
 * Build a FQuat from an axis and an angle. 
 * example: local Q = FQuat.FromAxisAndAngle(Axis, Angle)
 */
static int32 FQuat_FromAxisAndAngle(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector *Axis = (FVector*)GetCppInstanceFast(L, 1);
    if (!Axis)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Axis!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    float Angle = lua_tonumber(L, 2);

    if (NumParams > 2)
    {
        FQuat *Q = (FQuat*)GetCppInstanceFast(L, 3);
        if (Q)
        {
            *Q = FQuat(*Axis, Angle);
            lua_pushvalue(L, 3);
            return 1;
        }
    }

    void *Userdata = NewTypedUserdata(L, FQuat);
    new(Userdata) FQuat(*Axis, Angle);
    return 1;
}

static int32 FQuat_Set(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FQuat *V = (FQuat*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FQuat!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UnLua::TFieldSetter4<float>::Set(L, NumParams, &V->X);
    return 0;
}

static const luaL_Reg FQuatLib[] =
{
    { "Normalize", FQuat_Normalize },
    { "FromAxisAndAngle", FQuat_FromAxisAndAngle },
    { "Set", FQuat_Set },
    { "Mul", UnLua::TMathCalculation<FQuat, UnLua::TMul<FQuat>, true, UnLua::TMul<FQuat, float>>::Calculate },
    { "__mul", UnLua::TMathCalculation<FQuat, UnLua::TMul<FQuat>, false, UnLua::TMul<FQuat, float>>::Calculate },
    { "__tostring", UnLua::TMathUtils<FQuat>::ToString },
    { "__call", FQuat_New },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(FQuat)
    ADD_FUNCTION(GetNormalized)
    ADD_FUNCTION(IsNormalized)
    ADD_FUNCTION(Size)
    ADD_FUNCTION(SizeSquared)
    ADD_FUNCTION(ToAxisAndAngle)
    ADD_FUNCTION(Inverse)
    ADD_FUNCTION(RotateVector)
    ADD_FUNCTION(UnrotateVector)
    ADD_FUNCTION(GetAxisX)
    ADD_FUNCTION(GetAxisY)
    ADD_FUNCTION(GetAxisZ)
    ADD_FUNCTION(GetForwardVector)
    ADD_FUNCTION(GetRightVector)
    ADD_FUNCTION(GetUpVector)
    ADD_STATIC_FUNCTION(Slerp)
    ADD_NAMED_FUNCTION("ToEuler", Euler)
    ADD_NAMED_FUNCTION("ToRotator", Rotator)
    ADD_CONST_FUNCTION_EX("__add", FQuat, operator+, const FQuat&)
    ADD_CONST_FUNCTION_EX("__sub", FQuat, operator-, const FQuat&)
    ADD_CONST_FUNCTION_EX("__div", FQuat, operator/, const float)
    ADD_FUNCTION_EX("Add", FQuat, operator+=, const FQuat&)
    ADD_FUNCTION_EX("Sub", FQuat, operator-=, const FQuat&)
    ADD_FUNCTION_EX("Div", FQuat, operator/=, const float)
    ADD_LIB(FQuatLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FQuat)
