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

static int32 FIntPoint_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FIntPoint);
    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FIntPoint(ForceInitToZero);
            break;
        }
    case 2:
        {
            const int32& XY = lua_tointeger(L, 2);
            new(Userdata) FIntPoint(XY);
            break;
        }
    case 3:
        {
            const int32& X = lua_tointeger(L, 2);
            const int32& Y = lua_tointeger(L, 3);
            new(Userdata) FIntPoint(X, Y);
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

static int32 FIntPoint_Set(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FIntPoint* V = (FIntPoint*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FIntPoint!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UnLua::TFieldSetter2<int32>::Set(L, NumParams, &V->X);
    return 0;
}

static int32 FIntPoint_UNM(lua_State* L)
{
    FIntPoint* V = (FIntPoint*)GetCppInstanceFast(L, 1);
    if (!V)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FIntPoint!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void* Userdata = NewTypedUserdata(L, FIntPoint);
    new(Userdata) FIntPoint(-V->X, -V->Y);
    return 1;
}

static const luaL_Reg FIntPointLib[] =
{
    {"Set", FIntPoint_Set},
    {"Add", UnLua::TMathCalculation<FIntPoint, UnLua::TAdd<int32>, true>::Calculate},
    {"Sub", UnLua::TMathCalculation<FIntPoint, UnLua::TSub<int32>, true>::Calculate},
    {"Mul", UnLua::TMathCalculation<FIntPoint, UnLua::TMul<int32>, true>::Calculate},
    {"Div", UnLua::TMathCalculation<FIntPoint, UnLua::TDiv<int32>, true>::Calculate},
    {"__add", UnLua::TMathCalculation<FIntPoint, UnLua::TAdd<int32>>::Calculate},
    {"__sub", UnLua::TMathCalculation<FIntPoint, UnLua::TSub<int32>>::Calculate},
    {"__mul", UnLua::TMathCalculation<FIntPoint, UnLua::TMul<int32>>::Calculate},
    {"__div", UnLua::TMathCalculation<FIntPoint, UnLua::TDiv<int32>>::Calculate},
    {"__tostring", UnLua::TMathUtils<FIntPoint>::ToString},
    {"__unm", FIntPoint_UNM},
    {"__call", FIntPoint_New},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(FIntPoint)
    ADD_FUNCTION(Size)
    ADD_FUNCTION(SizeSquared)
    ADD_LIB(FIntPointLib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FIntPoint)
