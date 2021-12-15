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

template <typename T>
struct TPrimitiveTypeWrapper
{
    friend uint32 GetTypeHash(TPrimitiveTypeWrapper<T> In)
    {
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
        static_assert(TModels<CGetTypeHashable, T>::Value, "type must support GetTypeHash()!");
#else
        static_assert(THasGetTypeHash<T>::Value, "type must support GetTypeHash()!");
#endif
        return GetTypeHash(In.Value);
    }

    explicit TPrimitiveTypeWrapper(T InValue) : Value(InValue) {}

    T Value;
};

template <typename T>
struct TAggregateTypeWrapper
{
    friend uint32 GetTypeHash(const TAggregateTypeWrapper<T> &In)
    {
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
        static_assert(TModels<CGetTypeHashable, T>::Value, "type must support GetTypeHash()!");
#else
        static_assert(THasGetTypeHash<T>::Value, "type must support GetTypeHash()!");
#endif
        return GetTypeHash(In.Value);
    }

    explicit TAggregateTypeWrapper(const T &InValue) : Value(InValue) {}

    T Value;
};

#define EXPORT_PRIMITIVE_TYPE(Type, WrapperType, ArgType) \
    DEFINE_NAMED_TYPE(#Type, WrapperType) \
    BEGIN_EXPORT_CLASS_EX(false, Type, , WrapperType, nullptr, ArgType) \
        ADD_PROPERTY(Value) \
    END_EXPORT_CLASS(Type) \
    IMPLEMENT_EXPORTED_CLASS(Type) \
    ADD_TYPE_INTERFACE(Type)

/**
 * Macros to export primitive types
 */
EXPORT_PRIMITIVE_TYPE(int8, TPrimitiveTypeWrapper<int8>, int8)
EXPORT_PRIMITIVE_TYPE(int16, TPrimitiveTypeWrapper<int16>, int16)
EXPORT_PRIMITIVE_TYPE(int32, TPrimitiveTypeWrapper<int32>, int32)
EXPORT_PRIMITIVE_TYPE(int64, TPrimitiveTypeWrapper<int64>, int64)
EXPORT_PRIMITIVE_TYPE(uint8, TPrimitiveTypeWrapper<uint8>, uint8)
EXPORT_PRIMITIVE_TYPE(uint16, TPrimitiveTypeWrapper<uint16>, uint16)
EXPORT_PRIMITIVE_TYPE(uint32, TPrimitiveTypeWrapper<uint32>, uint32)
EXPORT_PRIMITIVE_TYPE(uint64, TPrimitiveTypeWrapper<uint64>, uint64)
EXPORT_PRIMITIVE_TYPE(float, TPrimitiveTypeWrapper<float>, float)
EXPORT_PRIMITIVE_TYPE(double, TPrimitiveTypeWrapper<double>, double)
EXPORT_PRIMITIVE_TYPE(bool, TPrimitiveTypeWrapper<bool>, bool)
EXPORT_PRIMITIVE_TYPE(FName, TPrimitiveTypeWrapper<FName>, FName)
EXPORT_PRIMITIVE_TYPE(FString, TAggregateTypeWrapper<FString>, const FString&)
//EXPORT_PRIMITIVE_TYPE(FText, TAggregateTypeWrapper<FText>, const FText&)
