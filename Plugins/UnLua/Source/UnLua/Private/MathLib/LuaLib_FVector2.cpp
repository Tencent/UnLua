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

static int32 FVector2D_New(lua_State *L)
{
    void *Userdata = NewTypedUserdata(L, FVector2D);
    FVector2D *V = new(Userdata) FVector2D(0.0f, 0.0f);
    UnLua::TFieldSetter2<float>::Set(L, lua_gettop(L), &V->X);
    return 1;
}

static int32 FVector2D_Set(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector2D *V = (FVector2D*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector2D!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UnLua::TFieldSetter2<float>::Set(L, NumParams, &V->X);
    return 0;
}

static int32 FVector2D_Normalize(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector2D *V = (FVector2D*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector2D!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    V->Normalize();
    return 0;
}

static int32 FVector2D_IsNormalized(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FVector2D *V = (FVector2D*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector2D!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_pushboolean(L, FMath::Abs(1.f - V->SizeSquared()) < THRESH_VECTOR_NORMALIZED);
    return 1;
}

static int32 FVector2D_UNM(lua_State *L)
{
    FVector2D *V = (FVector2D*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FVector2D!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Userdata = NewTypedUserdata(L, FVector2D);
    new(Userdata) FVector2D(-(*V));
    return 1;
}

static const luaL_Reg FVector2DLib[] =
{
    { "Set", FVector2D_Set },
    { "Normalize", FVector2D_Normalize },
    { "IsNormalized", FVector2D_IsNormalized },
    { "Add", UnLua::TMathCalculation<FVector2D, UnLua::TAdd<float>, true>::Calculate },
    { "Sub", UnLua::TMathCalculation<FVector2D, UnLua::TSub<float>, true>::Calculate },
    { "Mul", UnLua::TMathCalculation<FVector2D, UnLua::TMul<float>, true>::Calculate },
    { "Div", UnLua::TMathCalculation<FVector2D, UnLua::TDiv<float>, true>::Calculate },
    { "__add", UnLua::TMathCalculation<FVector2D, UnLua::TAdd<float>>::Calculate },
    { "__sub", UnLua::TMathCalculation<FVector2D, UnLua::TSub<float>>::Calculate },
    { "__mul", UnLua::TMathCalculation<FVector2D, UnLua::TMul<float>>::Calculate },
    { "__div", UnLua::TMathCalculation<FVector2D, UnLua::TDiv<float>>::Calculate },
    { "__tostring", UnLua::TMathUtils<FVector2D>::ToString },
    { "__unm", FVector2D_UNM },
    { "__call", FVector2D_New },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(FVector2D)
    ADD_CONST_FUNCTION_EX("Dot", float, operator|, const FVector2D&)
    ADD_CONST_FUNCTION_EX("Cross", float, operator^, const FVector2D&)
    ADD_FUNCTION(Size)
    ADD_FUNCTION(SizeSquared)
    ADD_STATIC_FUNCTION_EX("Dist", float, Distance, const FVector2D&, const FVector2D&)
    ADD_STATIC_FUNCTION(DistSquared)
    ADD_LIB(FVector2DLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FVector2D)
