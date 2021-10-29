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

static int32 FIntVector_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FIntVector);

    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FIntVector(ForceInitToZero);
            break;
        }
    case 2:
        {
            const int32& XYZ = lua_tointeger(L, 2);
            new(Userdata) FIntVector(XYZ);
            break;
        }
    case 4:
        {
            const int32& X = lua_tointeger(L, 2);
            const int32& Y = lua_tointeger(L, 3);
            const int32& Z = lua_tointeger(L, 4);
            new(Userdata) FIntVector(X, Y, Z);
            break;
        }
    default:
        {
            UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }
    }
    return 1;
}

static int32 FIntVector_Set(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FIntVector* V = (FIntVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FIntVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UnLua::TFieldSetter3<int32>::Set(L, NumParams, &V->X);
    return 0;
}

static int32 FIntVector_SizeSquared(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FIntVector* V = (FIntVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FIntVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 S = V->X * V->X + V->Y * V->Y + V->Z * V->Z;
    lua_pushnumber(L, S);
    return 1;
}

static int32 FIntVector_UNM(lua_State* L)
{
    FIntVector* V = (FIntVector*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FIntVector!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void* Userdata = NewTypedUserdata(L, FIntVector);
    new(Userdata) FIntVector(-V->X, -V->Y, -V->Z);
    return 1;
}

static const luaL_Reg FIntVectorLib[] =
{
    {"Set", FIntVector_Set},
    {"SizeSquared", FIntVector_SizeSquared},
    {"Add", UnLua::TMathCalculation<FIntVector, UnLua::TAdd<int32>, true>::Calculate},
    {"Sub", UnLua::TMathCalculation<FIntVector, UnLua::TSub<int32>, true>::Calculate},
    {"Mul", UnLua::TMathCalculation<FIntVector, UnLua::TMul<int32>, true>::Calculate},
    {"Div", UnLua::TMathCalculation<FIntVector, UnLua::TDiv<int32>, true>::Calculate},
    {"__add", UnLua::TMathCalculation<FIntVector, UnLua::TAdd<int32>>::Calculate},
    {"__sub", UnLua::TMathCalculation<FIntVector, UnLua::TSub<int32>>::Calculate},
    {"__mul", UnLua::TMathCalculation<FIntVector, UnLua::TMul<int32>>::Calculate},
    {"__div", UnLua::TMathCalculation<FIntVector, UnLua::TDiv<int32>>::Calculate},
    {"__tostring", UnLua::TMathUtils<FIntVector>::ToString},
    {"__unm", FIntVector_UNM},
    {"__call", FIntVector_New},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(FIntVector)
    ADD_FUNCTION(Size)
    ADD_LIB(FIntVectorLib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FIntVector)
