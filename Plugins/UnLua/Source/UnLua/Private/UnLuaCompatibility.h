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
#include "Runtime/Launch/Resources/Version.h"


//RPG3D MOD
#define UNLUA_UE_VER (ENGINE_MAJOR_VERSION * 100 + ENGINE_MINOR_VERSION)
//RPG3D

#if UNLUA_UE_VER < 419
#define DEFINE_FUNCTION(func) void func( FFrame& Stack, RESULT_DECL )
#define FNativeFuncPtr Native
#endif

#if UNLUA_UE_VER < 420
#define EPropertyFlags uint64
#define GET_INPUT_ACTION_NAME(IAB) IAB.ActionName
#define IS_INPUT_ACTION_PAIRED(IAB) IAB.bPaired
#else
#define GET_INPUT_ACTION_NAME(IAB) IAB.GetActionName()
#define IS_INPUT_ACTION_PAIRED(IAB) IAB.IsPaired()
#endif

#if UNLUA_UE_VER < 425
#define CastField Cast
#define GetPropertyOuter(Property) (Property)->GetOuter()
#define GetChildProperties(Function) (Function)->Children

typedef UProperty FProperty;
typedef UByteProperty FByteProperty;
typedef UInt8Property FInt8Property;
typedef UInt16Property FInt16Property;
typedef UIntProperty FIntProperty;
typedef UInt64Property FInt64Property;
typedef UUInt16Property FUInt16Property;
typedef UUInt32Property FUInt32Property;
typedef UUInt64Property FUInt64Property;
typedef UFloatProperty FFloatProperty;
typedef UDoubleProperty FDoubleProperty;
typedef UNumericProperty FNumericProperty;
typedef UEnumProperty FEnumProperty;
typedef UBoolProperty FBoolProperty;
typedef UObjectPropertyBase FObjectPropertyBase;
typedef UObjectProperty FObjectProperty;
typedef UClassProperty FClassProperty;
typedef UWeakObjectProperty FWeakObjectProperty;
typedef ULazyObjectProperty FLazyObjectProperty;
typedef USoftObjectProperty FSoftObjectProperty;
typedef USoftClassProperty FSoftClassProperty;
typedef UInterfaceProperty FInterfaceProperty;
typedef UNameProperty FNameProperty;
typedef UStrProperty FStrProperty;
typedef UTextProperty FTextProperty;
typedef UArrayProperty FArrayProperty;
typedef UMapProperty FMapProperty;
typedef USetProperty FSetProperty;
typedef UStructProperty FStructProperty;
typedef UDelegateProperty FDelegateProperty;
typedef UMulticastDelegateProperty FMulticastDelegateProperty;
#if UNLUA_UE_VER > 422
typedef UMulticastInlineDelegateProperty FMulticastInlineDelegateProperty;
typedef UMulticastSparseDelegateProperty FMulticastSparseDelegateProperty;
#endif
#else
#define GetPropertyOuter(Property) (Property)->GetOwnerUObject()
#define GetChildProperties(Function) (Function)->ChildProperties
#endif


template <typename T>
struct TMulticastDelegateTraits
{
    static const char* GetName() { return "FMulticastSparseDelegate"; }

    static void AddDelegate(FMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue)
    {
#if UNLUA_UE_VER > 422
        Property->AddDelegate(Delegate, nullptr, PropertyValue);
#endif
    }

    static void RemoveDelegate(FMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue)
    {
#if UNLUA_UE_VER > 422
        Property->RemoveDelegate(Delegate, nullptr, PropertyValue);
#endif
    }

    static void ClearDelegate(FMulticastDelegateProperty *Property, void *PropertyValue)
    {
#if UNLUA_UE_VER > 422
        Property->ClearDelegate(nullptr, PropertyValue);
#endif
    }

    static FMulticastScriptDelegate* GetMulticastDelegate(FMulticastDelegateProperty *Property, void *PropertyValue)
    {
#if UNLUA_UE_VER > 422
        return (FMulticastScriptDelegate*)Property->GetMulticastDelegate(PropertyValue);
#else
        return nullptr;
#endif
    }
};

template <> struct TMulticastDelegateTraits<FMulticastScriptDelegate>
{
    static const char* GetName() { return "FMulticastScriptDelegate"; }
    static void AddDelegate(FMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->AddUnique(Delegate); }
    static void RemoveDelegate(FMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->Remove(Delegate); }
    static void ClearDelegate(FMulticastDelegateProperty *Property, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->Clear(); }
    static FMulticastScriptDelegate* GetMulticastDelegate(FMulticastDelegateProperty *Property, void *PropertyValue) { return (FMulticastScriptDelegate*)PropertyValue; }
};
