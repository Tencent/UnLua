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

#pragma once

#include "LuaCore.h"
#include "UnLuaCompatibility.h"

static uint64 GetTypeHash(lua_State *L, int32 Index)
{
    if (lua_getmetatable(L, Index) == 0)
    {
        return 0;
    }
    uint64 Hash = 0;
    lua_pushstring(L, "TypeHash");
    lua_rawget(L, -2);
    if (lua_type(L, -1) == LUA_TNUMBER)
    {
        Hash = lua_tonumber(L, -1);
    }
    lua_pop(L, 2);
    return Hash;
}

namespace UnLua
{

    /**
     * Helper to set 2-fields struct
     */
    template <typename T>
    struct TFieldSetter2
    {
        static void Set(lua_State *L, int32 NumParams, T *V)
        {
            if (NumParams > 1)
            {
                V[0] = (T)lua_tonumber(L, 2);
            }
            if (NumParams > 2)
            {
                V[1] = (T)lua_tonumber(L, 3);
            }
        }
    };

    /**
     * Helper to set 3-fields struct
     */
    template <typename T>
    struct TFieldSetter3
    {
        static void Set(lua_State *L, int32 NumParams, T *V)
        {
            TFieldSetter2<T>::Set(L, NumParams, V);
            if (NumParams > 3)
            {
                V[2] = (T)lua_tonumber(L, 4);
            }
        }
    };

    /**
     * Helper to set 4-fields struct
     */
    template <typename T>
    struct TFieldSetter4
    {
        static void Set(lua_State *L, int32 NumParams, T *V)
        {
            TFieldSetter3<T>::Set(L, NumParams, V);
            if (NumParams > 4)
            {
                V[3] = (T)lua_tonumber(L, 5);
            }
        }
    };

    /**
     * Operator for '+'
     */
    template <typename T, typename T1 = T>
    struct TAdd
    {
        T operator()(const T &A, const T1 &B)
        {
            return A + B;
        }
    };

    /**
     * Operator for '-'
     */
    template <typename T, typename T1 = T>
    struct TSub
    {
        T operator()(const T &A, const T1 &B)
        {
            return A - B;
        }
    };

    /**
     * Operator for '*'
     */
    template <typename T, typename T1 = T>
    struct TMul
    {
        T operator()(const T &A, const T1 &B)
        {
            return A * B;
        }
    };

    template <> struct TMul<FTransform, float>
    {
        FTransform operator()(const FTransform &A, float B)
        {
            return A * ScalarRegister(B);
        }
    };

    /**
     * Operator for '/'
     */
    template <typename T, typename T1 = T>
    struct TDiv
    {
        T operator()(const T &A, const T1 &B)
        {
            return A / B;
        }
    };

    /**
     * Helper to calculate 'Result = A operator B'
     */
    template <typename T, typename ScalarType, typename OperatorType, typename ScalarOperatorType, int32 NumFields>
    struct TMathCalculationHelper
    {
        static void Calculate(T *Result, const T *A, const T *B, OperatorType Operator)
        {
            Result[0] = Operator(A[0], B[0]);
            Result[1] = Operator(A[1], B[1]);
            Result[2] = Operator(A[2], B[2]);
        }

        static void Calculate(T *Result, const T *A, ScalarType B, ScalarOperatorType Operator)
        {
            Result[0] = Operator(A[0], B);
            Result[1] = Operator(A[1], B);
            Result[2] = Operator(A[2], B);
        }
    };

    /**
     * partial specialization for 1-fields structs
     */
    template <typename T, typename ScalarType, typename OperatorType, typename ScalarOperatorType>
    struct TMathCalculationHelper<T, ScalarType, OperatorType, ScalarOperatorType, 1>
    {
        static void Calculate(T *Result, const T *A, const T *B, OperatorType Operator)
        {
            *Result = Operator(*A, *B);
        }

        static void Calculate(T *Result, const T *A, ScalarType B, ScalarOperatorType Operator)
        {
            *Result = Operator(*A, B);
        }
    };

    /**
     * partial specialization for 2-fields structs
     */
    template <typename T, typename ScalarType, typename OperatorType, typename ScalarOperatorType>
    struct TMathCalculationHelper<T, ScalarType, OperatorType, ScalarOperatorType, 2>
    {
        static void Calculate(T *Result, const T *A, const T *B, OperatorType Operator)
        {
            Result[0] = Operator(A[0], B[0]);
            Result[1] = Operator(A[1], B[1]);
        }

        static void Calculate(T *Result, const T *A, ScalarType B, ScalarOperatorType Operator)
        {
            Result[0] = Operator(A[0], B);
            Result[1] = Operator(A[1], B);
        }
    };

    /**
     * partial specialization for 4-fields structs
     */
    template <typename T, typename ScalarType, typename OperatorType, typename ScalarOperatorType>
    struct TMathCalculationHelper<T, ScalarType, OperatorType, ScalarOperatorType, 4>
    {
        static void Calculate(T *Result, const T *A, const T *B, OperatorType Operator)
        {
            Result[0] = Operator(A[0], B[0]);
            Result[1] = Operator(A[1], B[1]);
            Result[2] = Operator(A[2], B[2]);
            Result[3] = Operator(A[3], B[3]);
        }

        static void Calculate(T *Result, const T *A, ScalarType B, ScalarOperatorType Operator)
        {
            Result[0] = Operator(A[0], B);
            Result[1] = Operator(A[1], B);
            Result[2] = Operator(A[2], B);
            Result[3] = Operator(A[3], B);
        }
    };

    /**
     * Traits classes
     */
    template <typename T>
    struct TMathTypeTraits
    {
        typedef unluaReal FieldType;
        typedef unluaReal ScalarType;
        enum { NUM_FIELDS = 3 };
    };

    template<> struct TMathTypeTraits<FIntPoint>
    {
        typedef int32 FieldType;
        typedef int32 ScalarType;
        enum { NUM_FIELDS = 2 };
    };

    template<> struct TMathTypeTraits<FIntVector>
    {
        typedef int32 FieldType;
        typedef int32 ScalarType;
        enum { NUM_FIELDS = 3 };
    };

    template<> struct TMathTypeTraits<FLinearColor>
    {
        typedef float FieldType;
        typedef float ScalarType;
        enum { NUM_FIELDS = 4 };
    };

    template<> struct TMathTypeTraits<FVector2D>
    {
        typedef unluaReal FieldType;
        typedef unluaReal ScalarType;
        enum { NUM_FIELDS = 2 };
    };

    template<> struct TMathTypeTraits<FVector4>
    {
        typedef unluaReal FieldType;
        typedef unluaReal ScalarType;
        enum { NUM_FIELDS = 4 };
    };

    template<> struct TMathTypeTraits<FQuat>
    {
        typedef FQuat FieldType;
        typedef unluaReal ScalarType;
        enum { NUM_FIELDS = 1 };
    };

    template<> struct TMathTypeTraits<FTransform>
    {
        typedef FTransform FieldType;
        typedef unluaReal ScalarType;
        enum { NUM_FIELDS = 1 };
    };

    /**
     * Helper to get result address
     */
    template <typename T, bool bAssignment>
    struct TResultHelper
    {
        static T* GetResult(lua_State *L, T *A)
        {
            void *Userdata = NewUserdataWithPadding(L, sizeof(T), UnLua::TType<T>::GetName(), CalcUserdataPadding<T>());
            T *V = new(Userdata) T;
            return V;
        }
    };

    /**
     * Helper to get result address for += / -= / *= / /=, partial specialization
     */
    template <typename T>
    struct TResultHelper<T, true>
    {
        static T* GetResult(lua_State *L, T *A)
        {
            return A;
        }
    };

    /**
     * Helper to do math calculation
     */
    template <typename T, typename OperatorType, bool bAssignment = false, typename ScalarOperatorType = OperatorType>
    struct TMathCalculation
    {
        typedef typename TMathTypeTraits<T>::FieldType FT;
        typedef typename TMathTypeTraits<T>::ScalarType ST;

        static int32 Calculate(lua_State *L)
        {
            int32 NumParams = lua_gettop(L);
            if (NumParams != 2)
            {
                UE_LOG(LogUnLua, Log, TEXT("Invalid parameters!"));
                return 0;
            }

            T *A = (T*)GetCppInstanceFast(L, 1);
            if (!A)
            {
                UE_LOG(LogUnLua, Log, TEXT("Invalid parameter A!"));
                return 0;
            }

            int32 ParamType = lua_type(L, 2);
            if (ParamType != LUA_TUSERDATA && ParamType != LUA_TNUMBER)
            {
                UE_LOG(LogUnLua, Log, TEXT("Invalid parameter B!"));
                return 0;
            }

            T *Result = TResultHelper<T, bAssignment>::GetResult(L, A);
            switch (ParamType)
            {
            case LUA_TUSERDATA:
                {
                    uint64 Type1 = GetTypeHash(L, 1);
                    uint64 Type2 = GetTypeHash(L, 2);
                    if (!Type1 || !Type2 || Type1 != Type2)
                    {
                        UE_LOG(LogUnLua, Error, TEXT("Invalid parameters, incompatible types!"));
                        return 0;
                    }
                    T *B = (T*)GetCppInstanceFast(L, 2);
                    TMathCalculationHelper<FT, ST, OperatorType, ScalarOperatorType, TMathTypeTraits<T>::NUM_FIELDS>::Calculate(reinterpret_cast<FT*>(Result), reinterpret_cast<FT*>(A), reinterpret_cast<FT*>(B), OperatorType());
                }
                break;
            case LUA_TNUMBER:
                {
                    float B = lua_tonumber(L, 2);
                    TMathCalculationHelper<FT, ST, OperatorType, ScalarOperatorType, TMathTypeTraits<T>::NUM_FIELDS>::Calculate(reinterpret_cast<FT*>(Result), reinterpret_cast<FT*>(A), (ST)B, ScalarOperatorType());
                }
                break;
            }
            return bAssignment ? 0 : 1;
        }
    };

    template <typename T> FString ToStringWrapper(T *A) { return A->ToString(); }

    template <typename T>
    struct TMathUtils
    {
        static int32 ToString(lua_State *L)
        {
            int32 NumParams = lua_gettop(L);
            if (NumParams != 1)
            {
                UE_LOG(LogUnLua, Log, TEXT("Invalid parameters for __tostring!"));
                return 0;
            }

            T *A = (T*)GetCppInstanceFast(L, 1);
            if (!A)
            {
                int32 Type = luaL_getmetafield(L, 1, "__name");
                lua_pushfstring(L, "%s: %p", (Type == LUA_TSTRING) ? lua_tostring(L, -1) : lua_typename(L, lua_type(L, 1)), lua_topointer(L, 1));
                if (Type != LUA_TNIL)
                {
                    lua_remove(L, -2);
                }
                return 1;
            }

            lua_pushstring(L, TCHAR_TO_UTF8(*(ToStringWrapper(A))));
            return 1;
        }
    };

} // namespace UnLua
