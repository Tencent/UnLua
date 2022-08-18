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

static int32 FVector4_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FVector4);

    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FVector4(ForceInitToZero);
            return 1;
        }
    case 2:
        {
            const unluaReal& X = lua_tonumber(L, 2);
            new(Userdata) FVector4(X);
            return 1;
        }
    case 3:
        {
            const unluaReal& X = lua_tonumber(L, 2);
            const unluaReal& Y = lua_tonumber(L, 3);
            new(Userdata) FVector4(X, Y);
            return 1;
        }
    case 4:
        {
            const unluaReal& X = lua_tonumber(L, 2);
            const unluaReal& Y = lua_tonumber(L, 3);
            const unluaReal& Z = lua_tonumber(L, 4);
            new(Userdata) FVector4(X, Y, Z);
            return 1;
        }
    case 5:
        {
            const unluaReal& X = lua_tonumber(L, 2);
            const unluaReal& Y = lua_tonumber(L, 3);
            const unluaReal& Z = lua_tonumber(L, 4);
            const unluaReal& W = lua_tonumber(L, 5);
            new(Userdata) FVector4(X, Y, Z, W);
            return 1;
        }
    }

    return luaL_error(L, "invalid parameters");
}

static int32 FVector4_Set(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
        return luaL_error(L, "invalid parameters");

    FVector4* V = (FVector4*)GetCppInstanceFast(L, 1);
    if (!V)
        return luaL_error(L, "invalid FVector4");

    UnLua::TFieldSetter4<unluaReal>::Set(L, NumParams, &V->X);
    return 0;
}

static int32 FVector4_UNM(lua_State* L)
{
    FVector4* V = (FVector4*)GetCppInstanceFast(L, 1);
    if (!V)
        return luaL_error(L, "invalid FVector4");

    void* Userdata = NewTypedUserdata(L, FVector4);
    new(Userdata) FVector4(-(*V));
    return 1;
}

static const luaL_Reg FVector4Lib[] =
{
    {"Set", FVector4_Set},
    {"Add", UnLua::TMathCalculation<FVector4, UnLua::TAdd<unluaReal>, true>::Calculate},
    {"Sub", UnLua::TMathCalculation<FVector4, UnLua::TSub<unluaReal>, true>::Calculate},
    {"Mul", UnLua::TMathCalculation<FVector4, UnLua::TMul<unluaReal>, true>::Calculate},
    {"Div", UnLua::TMathCalculation<FVector4, UnLua::TDiv<unluaReal>, true>::Calculate},
    {"__add", UnLua::TMathCalculation<FVector4, UnLua::TAdd<unluaReal>>::Calculate},
    {"__sub", UnLua::TMathCalculation<FVector4, UnLua::TSub<unluaReal>>::Calculate},
    {"__mul", UnLua::TMathCalculation<FVector4, UnLua::TMul<unluaReal>>::Calculate},
    {"__div", UnLua::TMathCalculation<FVector4, UnLua::TDiv<unluaReal>>::Calculate},
    {"__tostring", UnLua::TMathUtils<FVector4>::ToString},
    {"__unm", FVector4_UNM},
    {"__call", FVector4_New},
    {nullptr, nullptr}
};

#if ENGINE_MAJOR_VERSION < 5
float Dot3(const FVector4& V1, const FVector4& V2);
float Dot4(const FVector4& V1, const FVector4& V2);
#endif

BEGIN_EXPORT_REFLECTED_CLASS(FVector4)
#if ENGINE_MAJOR_VERSION < 5
    ADD_EXTERNAL_FUNCTION(float, Dot3, const FVector4&, const FVector4&)
    ADD_EXTERNAL_FUNCTION(float, Dot4, const FVector4&, const FVector4&)
#else
    ADD_EXTERNAL_FUNCTION_EX("Dot3", unluaReal, Dot3<unluaReal>, const FVector4&, const FVector4&)
#endif
    ADD_CONST_FUNCTION_EX("Cross", FVector4, operator^, const FVector4&)
    ADD_FUNCTION(Size)
    ADD_FUNCTION(Size3)
    ADD_FUNCTION(SizeSquared)
    ADD_FUNCTION(SizeSquared3)
    ADD_NAMED_FUNCTION("ToRotator", ToOrientationRotator)
    ADD_NAMED_FUNCTION("ToQuat", ToOrientationQuat)
    ADD_LIB(FVector4Lib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FVector4)
