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

static int32 FColor_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FColor);

    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FColor(ForceInitToZero);
            break;
        }
    case 2:
        {
            const uint32& ColorValue = lua_tointeger(L, 2);
            new(Userdata) FColor(ColorValue);
            break;
        }
    case 4:
        {
            const uint8& R = (uint8)lua_tointeger(L, 2);
            const uint8& G = (uint8)lua_tointeger(L, 3);
            const uint8& B = (uint8)lua_tointeger(L, 4);
            new(Userdata) FColor(R, G, B);
            break;
        }
    case 5:
        {
            const uint8& R = (uint8)lua_tointeger(L, 2);
            const uint8& G = (uint8)lua_tointeger(L, 3);
            const uint8& B = (uint8)lua_tointeger(L, 4);
            const uint8& A = (uint8)lua_tointeger(L, 5);
            new(Userdata) FColor(R, G, B, A);
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

static int32 FColor_Set(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FColor* A = (FColor*)GetCppInstanceFast(L, 1);
    if (!A)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FColor!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    if (NumParams > 1)
    {
        A->R = (uint8)lua_tonumber(L, 2);
    }
    if (NumParams > 2)
    {
        A->G = (uint8)lua_tonumber(L, 3);
    }
    if (NumParams > 3)
    {
        A->B = (uint8)lua_tonumber(L, 4);
    }
    if (NumParams > 4)
    {
        A->A = (uint8)lua_tonumber(L, 5);
    }
    return 0;
}

static int32 FColor_Add(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FColor* A = (FColor*)GetCppInstanceFast(L, 1);
    if (!A)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FColor A!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    FColor* B = (FColor*)GetCppInstanceFast(L, 2);
    if (!B)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FColor B!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FColor C = *A;
    C += (*B);

    void* Userdata = NewTypedUserdata(L, FColor);
    FColor* V = new(Userdata) FColor(C);
    return 1;
}

static const luaL_Reg FColorLib[] =
{
    {"Set", FColor_Set},
    {"__add", FColor_Add},
    {"__call", FColor_New},
    {"__tostring", UnLua::TMathUtils<FColor>::ToString},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(FColor)
    ADD_FUNCTION_EX("Add", void, operator+=, const FColor&)
    ADD_NAMED_FUNCTION("ToLinearColor", ReinterpretAsLinear)
    ADD_STATIC_PROPERTY(White)
    ADD_STATIC_PROPERTY(Black)
    ADD_LIB(FColorLib)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FColor)
