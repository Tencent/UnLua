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

static int32 FTransform_New(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    void* Userdata = NewTypedUserdata(L, FTransform);

    switch (NumParams)
    {
    case 1:
        {
            new(Userdata) FTransform();
            return 1;
        }
    case 2:
        {
            const FQuat* Rotation = (FQuat*)GetCppInstanceFast(L, 2);
            if (Rotation)
            {
                new(Userdata) FTransform(*Rotation);
                return 1;
            }

            // TODO: not supported currently
            // if (IsType(L, 2, UnLua::TType<FVector>()))
            // {
            //     const FVector* Translation = (FVector*)GetCppInstanceFast(L, 2);;
            //     if (Translation)
            //     {
            //         new(Userdata) FTransform(*Translation);
            //         return 1;
            //     }
            // }
            //
            // if (IsType(L, 2, UnLua::TType<FRotator>()))
            // {
            //     const FRotator* Rotation = (FRotator*)GetCppInstanceFast(L, 2);
            //     if (Rotation)
            //     {
            //         new(Userdata) FTransform(*Rotation);
            //         return 1;
            //     }
            // }
            //
            // if (IsType(L, 2, UnLua::TType<FQuat>()))
            // {
            //     const FQuat* Quat = (FQuat*)GetCppInstanceFast(L, 2);
            //     if (Quat)
            //     {
            //         new(Userdata) FTransform(*Quat);
            //         return 1;
            //     }
            // }

            break;
        }
    case 3:
        {
            const FQuat* Rotation = (FQuat*)GetCppInstanceFast(L, 2);
            const FVector* Translation = (FVector*)GetCppInstanceFast(L, 3);
            if (Rotation && Translation)
            {
                new(Userdata) FTransform(*Rotation, *Translation);
                return 1;
            }

            break;
        }
    case 4:
        {
            const FQuat* Rotation = (FQuat*)GetCppInstanceFast(L, 2);
            const FVector* Translation = (FVector*)GetCppInstanceFast(L, 3);
            const FVector* Scale = (FVector*)GetCppInstanceFast(L, 4);
            if (Rotation && Translation && Scale)
            {
                new(Userdata) FTransform(*Rotation, *Translation, *Scale);
                return 1;
            }

            break;
        }
    }

    return luaL_error(L, "invalid parameters");
}

static int32 FTransform_Blend(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 3)
        return luaL_error(L, "invalid parameters");

    FTransform* A = (FTransform*)GetCppInstanceFast(L, 1);
    if (!A)
        return luaL_error(L, "invalid FTransform A");

    FTransform* B = (FTransform*)GetCppInstanceFast(L, 2);
    if (!B)
        return luaL_error(L, "invalid FTransform B");

    FTransform* C = (FTransform*)GetCppInstanceFast(L, 3);
    if (!C)
        return luaL_error(L, "invalid FTransform C");

    const float Alpha = lua_tonumber(L, 4);
    A->Blend(*B, *C, Alpha);
    return 0;
}

static int32 FTransform_BlendWith(lua_State* L)
{
    const int32 NumParams = lua_gettop(L);
    if (NumParams < 3)
        return luaL_error(L, "invalid parameters");

    FTransform* A = (FTransform*)GetCppInstanceFast(L, 1);
    if (!A)
        return luaL_error(L, "invalid FTransform A");

    FTransform* B = (FTransform*)GetCppInstanceFast(L, 2);
    if (!B)
        return luaL_error(L, "invalid FTransform B");

    const float Alpha = lua_tonumber(L, 3);
    A->BlendWith(*B, Alpha);

    return 0;
}

static const luaL_Reg FTransformLib[] =
{
    {"Blend", FTransform_Blend},
    {"BlendWith", FTransform_BlendWith},
    {"Mul", UnLua::TMathCalculation<FTransform, UnLua::TMul<FTransform>, true, UnLua::TMul<FTransform, float>>::Calculate},
    {"__mul", UnLua::TMathCalculation<FTransform, UnLua::TMul<FTransform>, false, UnLua::TMul<FTransform, float>>::Calculate},
    {"__tostring", UnLua::TMathUtils<FTransform>::ToString},
    {"__call", FTransform_New},
    {nullptr, nullptr}
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
