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

#include "CoreUObject.h"
#include <type_traits>

namespace UnLua
{

    /**
     * Traits class which tests if a type is constructible
     */
    template <class T, class... Args> struct TIsConstructible { enum { Value = std::is_constructible<T, Args...>::value }; };
    template <class T> struct TIsCopyConstructible { enum { Value = std::is_copy_constructible<T>::value }; };
    template <class T> struct TIsDestructible { enum { Value = std::is_destructible<T>::value }; };


    /**
     * Traits class which tests if a type has equality operator
     */
    struct FHasEqualityOperatorImpl
    {
        template<class T1, class T2>
        static auto Identical(T1*) -> decltype(DeclVal<T1>() == DeclVal<T2>());
        template<class T1, class T2>
        static int32 Identical(...);
    };
    template<typename T1, typename T2 = T1> struct THasEqualityOperator { enum { Value = TIsSame<decltype(FHasEqualityOperatorImpl::Identical<T1, T2>(nullptr)), bool>::Value }; };


    /**
     * Traits class which tests if a type is const
     */
    template <typename T> struct TIsConstType { enum { Value = false }; };
    template <typename T> struct TIsConstType<const T> { enum { Value = true }; };
    template <typename T> struct TIsConstType<const T&> { enum { Value = true }; };
    template <typename T> struct TIsConstType<const T*> { enum { Value = true }; };
    template <typename T> struct TIsConstType<const volatile T> { enum { Value = true }; };
    template <typename T> struct TIsConstType<const volatile T&> { enum { Value = true }; };
    template <typename T> struct TIsConstType<const volatile T*> { enum { Value = true }; };


    /**
     * Traits class which tests if a type is primitive
     */
    template <typename T> struct TIsPrimitiveType { enum { Value = TOr<TIsEnum<T>, TIsArithmetic<T>>::Value }; };
    template <typename T> struct TIsPrimitiveType<const T> { enum { Value = TIsPrimitiveType<T>::Value }; };
    template <typename T> struct TIsPrimitiveType<volatile T> { enum { Value = TIsPrimitiveType<T>::Value }; };
    template <typename T> struct TIsPrimitiveType<const volatile T> { enum { Value = TIsPrimitiveType<T>::Value }; };
    template<> struct TIsPrimitiveType<long> { enum { Value = true }; };
    template<> struct TIsPrimitiveType<unsigned long> { enum { Value = true }; };
    template<> struct TIsPrimitiveType<char*> { enum { Value = true }; };
    template<> struct TIsPrimitiveType<FString> { enum { Value = true }; };
    template<> struct TIsPrimitiveType<FName> { enum { Value = true }; };
    template<> struct TIsPrimitiveType<FText> { enum { Value = true }; };


    /**
     * Traits class which tests if a type is primitive or pointer
     */
    template <typename T> struct TIsPrimitiveTypeOrPointer { enum { Value = TOr<TIsPrimitiveType<T>, TIsPointer<T>>::Value }; };


    /**
     * Traits class for a argument type
     */
    template <typename T> struct TArgTypeTraits
    {
        typedef typename TDecay<T>::Type RT;
        typedef typename TChooseClass<TIsPrimitiveTypeOrPointer<RT>::Value, RT, typename std::add_lvalue_reference<T>::type>::Result Type;
    };
    
    
    /**
     * Traits class to get a type from a tuple
     */
    template <uint32 I, typename T> struct TTupleType;
    template <uint32 I, typename T1, typename... T2> struct TTupleType<I, TTuple<T1, T2...>> : public TTupleType<I-1, TTuple<T2...>> {};
    template <typename T1, typename... T2> struct TTupleType<0, TTuple<T1, T2...>> { typedef T1 Type; };


    /**
     * Indices 
     */
    template <uint32... N> struct TIndices {};

    template <uint32 M, uint32... N> struct TOneBasedIndices : public TOneBasedIndices<M-1, M, N...> {};
    template <uint32... N> struct TOneBasedIndices<0, N...> { typedef TIndices<N...> Type; };

    template <uint32 M, uint32... N> struct TZeroBasedIndices : public TZeroBasedIndices<M-1, M-1, N...> {};
    template <uint32... N> struct TZeroBasedIndices<0, N...> { typedef TIndices<N...> Type; };


    /**
     * Get UScriptStruct from 'CoreUObject'
     *
     * @param Name - struct name
     * @return - UScriptStruct
     */
    FORCEINLINE UScriptStruct* GetScriptStructInCoreUObject(const TCHAR *Name)
    {
        static UPackage *CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));
        return FindObjectChecked<UScriptStruct>(CoreUObjectPkg, Name);
    }

    /**
     * Traits class to get UScriptStruct from a type
     */
    template< class T >
    struct TScriptStructTraits
    {
        static UScriptStruct* Get() { return T::StaticStruct(); }
    };

    template<> struct TScriptStructTraits<FRotator>
    {
        static UScriptStruct* Get() { return TBaseStructure<FRotator>::Get(); }
    };

    template<> struct TScriptStructTraits<FQuat>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("Quat")); }
    };

    template<> struct TScriptStructTraits<FTransform>
    {
        static UScriptStruct* Get() { return TBaseStructure<FTransform>::Get(); }
    };

    template<> struct TScriptStructTraits<FLinearColor>
    {
        static UScriptStruct* Get() { return TBaseStructure<FLinearColor>::Get(); }
    };

    template<> struct TScriptStructTraits<FColor>
    {
        static UScriptStruct* Get() { return TBaseStructure<FColor>::Get(); }
    };

    template<> struct TScriptStructTraits<FPlane>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("Plane")); }
    };

    template<> struct  TScriptStructTraits<FVector>
    {
        static UScriptStruct* Get() { return TBaseStructure<FVector>::Get(); }
    };

    template<> struct TScriptStructTraits<FVector2D>
    {
        static UScriptStruct* Get() { return TBaseStructure<FVector2D>::Get(); }
    };

    template<> struct TScriptStructTraits<FVector4>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("Vector4")); }
    };

    template<> struct TScriptStructTraits<FRandomStream>
    {
        static UScriptStruct* Get() { return TBaseStructure<FRandomStream>::Get(); }
    };

    template<> struct TScriptStructTraits<FGuid>
    {
        static UScriptStruct* Get() { return TBaseStructure<FGuid>::Get(); }
    };

    template<> struct TScriptStructTraits<FBox2D>
    {
        static UScriptStruct* Get() { return TBaseStructure<FBox2D>::Get(); }
    };

    template<> struct TScriptStructTraits<FFallbackStruct>
    {
        static UScriptStruct* Get() { return TBaseStructure<FFallbackStruct>::Get(); }
    };

    template<> struct TScriptStructTraits<FFloatRangeBound>
    {
        static UScriptStruct* Get() { return TBaseStructure<FFloatRangeBound>::Get(); }
    };

    template<> struct TScriptStructTraits<FFloatRange>
    {
        static UScriptStruct* Get() { return TBaseStructure<FFloatRange>::Get(); }
    };

    template<> struct TScriptStructTraits<FInt32RangeBound>
    {
        static UScriptStruct* Get() { return TBaseStructure<FInt32RangeBound>::Get(); }
    };

    template<> struct TScriptStructTraits<FInt32Range>
    {
        static UScriptStruct* Get() { return TBaseStructure<FInt32Range>::Get(); }
    };

    template<> struct TScriptStructTraits<FFloatInterval>
    {
        static UScriptStruct* Get() { return TBaseStructure<FFloatInterval>::Get(); }
    };

    template<> struct TScriptStructTraits<FInt32Interval>
    {
        static UScriptStruct* Get() { return TBaseStructure<FInt32Interval>::Get(); }
    };

    template<> struct TScriptStructTraits<FSoftObjectPath>
    {
        static UScriptStruct* Get() { return TBaseStructure<FSoftObjectPath>::Get(); }
    };

    template<> struct TScriptStructTraits<FSoftClassPath>
    {
        static UScriptStruct* Get() { return TBaseStructure<FSoftClassPath>::Get(); }
    };

    template<> struct TScriptStructTraits<FPrimaryAssetType>
    {
        static UScriptStruct* Get() { return TBaseStructure<FPrimaryAssetType>::Get(); }
    };

    template<> struct TScriptStructTraits<FPrimaryAssetId>
    {
        static UScriptStruct* Get() { return TBaseStructure<FPrimaryAssetId>::Get(); }
    };

    template<> struct TScriptStructTraits<FDateTime>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("DateTime")); }
    };

    template<> struct TScriptStructTraits<FIntVector>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("IntVector")); }
    };

    template<> struct TScriptStructTraits<FIntPoint>
    {
        static UScriptStruct* Get() { return GetScriptStructInCoreUObject(TEXT("IntPoint")); }
    };


    /**
     * Generic type
     */
    template <typename T, bool IsUObject = TPointerIsConvertibleFromTo<typename TDecay<T>::Type, UObject>::Value>
    struct TType
    {
        static const char* GetName()
        {
            UScriptStruct *ScriptStruct = TScriptStructTraits<T>::Get();
            static FTCHARToUTF8 Name(*FString::Printf(TEXT("F%s"), *ScriptStruct->GetName()));
            return Name.Get();
        }
    };

    template <typename T>
    struct TType<T, true>
    {
        static const char* GetName()
        {
            UClass *Class = T::StaticClass();
            static FTCHARToUTF8 Name(*FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName()));
            return Name.Get();
        }
    };

} // namespace UnLua

/**
 * Macros to define types
 */
#define DEFINE_TYPE(Type) \
    template<> struct UnLua::TType<Type, false> \
    { \
        static const char* GetName() { return #Type; } \
    };

#define DEFINE_NAMED_TYPE(Name, Type) \
    template<> struct UnLua::TType<Type, false> \
    { \
        static const char* GetName() { return Name; } \
    };

#define DEFINE_SMART_POINTER(SmartPtrType) \
    template <typename T> struct UnLua::TType<SmartPtrType<T, ESPMode::NotThreadSafe>, false> \
    { \
        static const char* GetName() { return #SmartPtrType; } \
    }; \
    template <typename T> struct UnLua::TType<SmartPtrType<T, ESPMode::ThreadSafe>, false> \
    { \
        static const char* GetName() { return #SmartPtrType; } \
    };

/**
 * Define primitive types
 */
DEFINE_TYPE(int8)
DEFINE_TYPE(int16)
DEFINE_TYPE(int32)
DEFINE_TYPE(int64)
DEFINE_TYPE(long)
DEFINE_TYPE(uint8)
DEFINE_TYPE(uint16)
DEFINE_TYPE(uint32)
DEFINE_TYPE(uint64)
DEFINE_NAMED_TYPE("ulong", unsigned long)
DEFINE_TYPE(float)
DEFINE_TYPE(double)
DEFINE_TYPE(bool)
DEFINE_TYPE(FString)
DEFINE_TYPE(FName)
DEFINE_TYPE(FText)
DEFINE_SMART_POINTER(TSharedPtr)
DEFINE_SMART_POINTER(TSharedRef)
DEFINE_SMART_POINTER(TWeakPtr)
