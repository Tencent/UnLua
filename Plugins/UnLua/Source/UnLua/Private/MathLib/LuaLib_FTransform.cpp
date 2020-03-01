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

static int32 FTransform_New(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    void *Userdata = NewTypedUserdata(L, FTransform);
    FTransform *V = new(Userdata) FTransform;
    if (NumParams > 1)
    {
        V->SetRotation(*((FQuat*)GetCppInstanceFast(L, 2)));
    }
    if (NumParams > 2)
    {
        V->SetTranslation(*((FVector*)GetCppInstanceFast(L, 3)));
    }
    if (NumParams > 3)
    {
        V->SetScale3D(*((FVector*)GetCppInstanceFast(L, 4)));
    }
    return 1;
}

static int32 FTransform_Blend(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 3)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FTransform *A = (FTransform*)GetCppInstanceFast(L, 1);
    if (!A)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FTransform A!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    FTransform *B = (FTransform*)GetCppInstanceFast(L, 2);
    if (!B)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FTransform B!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    if (NumParams > 3)
    {
        FTransform *C = (FTransform*)GetCppInstanceFast(L, 3);
        if (!C)
        {
            UE_LOG(LogUnLua, Log, TEXT("%s: Invalid FTransform C!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }
        float Alpha = lua_tonumber(L, 4);
        A->Blend(*B, *C, Alpha);
    }
    else
    {
        float Alpha = lua_tonumber(L, 3);
        A->BlendWith(*B, Alpha);
    }
    return 0;
}

static const luaL_Reg FTransformLib[] =
{
    { "Blend", FTransform_Blend },
    { "Mul", UnLua::TMathCalculation<FTransform, UnLua::TMul<FTransform>, true, UnLua::TMul<FTransform, float>>::Calculate },
    { "__mul", UnLua::TMathCalculation<FTransform, UnLua::TMul<FTransform>, false, UnLua::TMul<FTransform, float>>::Calculate },
    { "__tostring", UnLua::TMathUtils<FTransform>::ToString },
    { "__call", FTransform_New },
    { nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(FTransform)
    ADD_FUNCTION(Inverse)
    ADD_FUNCTION(TransformPosition)
    ADD_FUNCTION(TransformPositionNoScale)
    ADD_FUNCTION(InverseTransformPosition)
    ADD_FUNCTION(InverseTransformPositionNoScale)
    ADD_FUNCTION(TransformVector)
    ADD_FUNCTION(TransformVectorNoScale)
    ADD_FUNCTION(InverseTransformVector)
    ADD_FUNCTION(InverseTransformVectorNoScale)
    ADD_FUNCTION(TransformRotation)
    ADD_FUNCTION(InverseTransformRotation)
    ADD_CONST_FUNCTION_EX("__add", FTransform, operator+, const FTransform&)
    ADD_FUNCTION_EX("Add", FTransform&, operator+=, const FTransform&)
    ADD_LIB(FTransformLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FTransform)
