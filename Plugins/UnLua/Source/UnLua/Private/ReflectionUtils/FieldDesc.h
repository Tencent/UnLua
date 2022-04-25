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

#include "ClassDesc.h"

/**
 * Field descriptor
 */
class FFieldDesc
{
    friend class FClassDesc;

public:
    FFieldDesc()
        : QueryClass(nullptr), OuterClass(nullptr), FieldIndex(0)
    {}
    
    FORCEINLINE bool IsValid() const { return FieldIndex != 0; }

    FORCEINLINE bool IsProperty() const { return FieldIndex > 0; }

    FORCEINLINE bool IsFunction() const { return FieldIndex < 0; }

    FORCEINLINE bool IsInherited() const { return OuterClass != QueryClass; }

    FORCEINLINE TSharedPtr<FPropertyDesc> AsProperty() const { return FieldIndex > 0 ? OuterClass->GetProperty(FieldIndex - 1) : nullptr; }

    FORCEINLINE TSharedPtr<FFunctionDesc> AsFunction() const { return FieldIndex < 0 ? OuterClass->GetFunction(-FieldIndex - 1) : nullptr; }

    FORCEINLINE FString GetOuterName() const { return OuterClass ? OuterClass->GetName() : TEXT(""); }

    FClassDesc *QueryClass;
    FClassDesc *OuterClass;
    int32 FieldIndex;   // index in FClassDesc
};