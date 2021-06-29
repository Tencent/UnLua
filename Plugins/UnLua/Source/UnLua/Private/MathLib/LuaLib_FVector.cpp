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

static int32 FVector_New(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    void *Userdata = NewTypedUserdata(L, FVector);
    FVector *V = new(Userdata) FVector(0.0f);
    UnLua::TFieldSetter3<float>::Set(L, NumParams, &V->X);
    return 1;
}

static int32 FVector_Set(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector *V = (FVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UnLua::TFieldSetter3<float>::Set(L, NumParams, &V->X);
    return 0;
}

static int32 FVector_Normalize(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector *V = (FVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_pushboolean(L, V->Normalize());
    return 1;
}

static int32 FVector_UNM(lua_State *L)
{
    FVector *V = (FVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Userdata = NewTypedUserdata(L, FVector);
    new(Userdata) FVector(-(*V));
    return 1;
}

static const luaL_Reg FVectorLib[] =
{
    { "Set", FVector_Set },
    { "Normalize", FVector_Normalize },
    { "Add", UnLua::TMathCalculation<FVector, UnLua::TAdd<float>, true>::Calculate },
    { "Sub", UnLua::TMathCalculation<FVector, UnLua::TSub<float>, true>::Calculate },
    { "Mul", UnLua::TMathCalculation<FVector, UnLua::TMul<float>, true>::Calculate },
    { "Div", UnLua::TMathCalculation<FVector, UnLua::TDiv<float>, true>::Calculate },
    { "__add", UnLua::TMathCalculation<FVector, UnLua::TAdd<float>>::Calculate },
    { "__sub", UnLua::TMathCalculation<FVector, UnLua::TSub<float>>::Calculate },
    { "__mul", UnLua::TMathCalculation<FVector, UnLua::TMul<float>>::Calculate },
    { "__div", UnLua::TMathCalculation<FVector, UnLua::TDiv<float>>::Calculate },
    { "__tostring", UnLua::TMathUtils<FVector>::ToString },
    { "__unm", FVector_UNM },
    { "__call", FVector_New },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(FVector)
    ADD_CONST_FUNCTION_EX("Dot", float, operator|, const FVector&)
    ADD_CONST_FUNCTION_EX("Cross", FVector, operator^, const FVector&)
    ADD_FUNCTION(Size)
    ADD_FUNCTION(Size2D)
    ADD_FUNCTION(SizeSquared)
    ADD_FUNCTION(SizeSquared2D)
    ADD_STATIC_FUNCTION(Dist)
    ADD_STATIC_FUNCTION(Dist2D)
    ADD_STATIC_FUNCTION(DistSquared)
    ADD_STATIC_FUNCTION(DistSquared2D)
    ADD_FUNCTION(IsNormalized)
    ADD_FUNCTION(CosineAngle2D)
    ADD_FUNCTION(RotateAngleAxis)
    ADD_NAMED_FUNCTION("ToRotator", ToOrientationRotator)
    ADD_NAMED_FUNCTION("ToQuat", ToOrientationQuat)
    ADD_LIB(FVectorLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FVector)
