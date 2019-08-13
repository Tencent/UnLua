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

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MINOR_VERSION < 19
#define DEFINE_FUNCTION(func) void func( FFrame& Stack, RESULT_DECL )
#define FNativeFuncPtr Native
#endif

#if ENGINE_MINOR_VERSION < 20
#define EPropertyFlags uint64
#define GET_INPUT_ACTION_NAME(IAB) IAB.ActionName
#define IS_INPUT_ACTION_PAIRED(IAB) IAB.bPaired
#else
#define GET_INPUT_ACTION_NAME(IAB) IAB.GetActionName()
#define IS_INPUT_ACTION_PAIRED(IAB) IAB.IsPaired()
#endif


template <typename T>
struct TMulticastDelegateTraits
{
    static const char* GetName() { return "FMulticastSparseDelegate"; }

    static void AddDelegate(UMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue)
    {
#if ENGINE_MINOR_VERSION > 22
        Property->AddDelegate(Delegate, nullptr, PropertyValue);
#endif
    }

    static void RemoveDelegate(UMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue)
    {
#if ENGINE_MINOR_VERSION > 22
        Property->RemoveDelegate(Delegate, nullptr, PropertyValue);
#endif
    }

    static void ClearDelegate(UMulticastDelegateProperty *Property, void *PropertyValue)
    {
#if ENGINE_MINOR_VERSION > 22
        Property->ClearDelegate(nullptr, PropertyValue);
#endif
    }

    static FMulticastScriptDelegate* GetMulticastDelegate(UMulticastDelegateProperty *Property, void *PropertyValue)
    {
#if ENGINE_MINOR_VERSION > 22
        return (FMulticastScriptDelegate*)Property->GetMulticastDelegate(PropertyValue);
#else
        return nullptr;
#endif
    }
};

template <> struct TMulticastDelegateTraits<FMulticastScriptDelegate>
{
    static const char* GetName() { return "FMulticastScriptDelegate"; }
    static void AddDelegate(UMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->AddUnique(Delegate); }
    static void RemoveDelegate(UMulticastDelegateProperty *Property, FScriptDelegate Delegate, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->Remove(Delegate); }
    static void ClearDelegate(UMulticastDelegateProperty *Property, void *PropertyValue) { ((FMulticastScriptDelegate*)PropertyValue)->Clear(); }
    static FMulticastScriptDelegate* GetMulticastDelegate(UMulticastDelegateProperty *Property, void *PropertyValue) { return (FMulticastScriptDelegate*)PropertyValue; }
};
