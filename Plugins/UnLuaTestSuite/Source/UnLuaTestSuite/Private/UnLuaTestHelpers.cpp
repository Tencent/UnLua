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

#include "UnLuaTestHelpers.h"
#include "UnLua.h"
#include "UnLuaEx.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_EXPORT_CLASS(FUnLuaTestLib)
    ADD_SHARED_PTR_CONSTRUCTOR(ESPMode::NotThreadSafe)
    ADD_STATIC_FUNCTION(TestForBaseSpec1)
    ADD_STATIC_FUNCTION(TestForBaseSpec2)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FUnLuaTestLib)

BEGIN_EXPORT_ENUM_EX(EScopedEnum::Type, EScopedEnum)
    ADD_SCOPED_ENUM_VALUE(Value1)
    ADD_SCOPED_ENUM_VALUE(Value2)
END_EXPORT_ENUM(EScopedEnum)

BEGIN_EXPORT_ENUM(EEnumClass)
    ADD_SCOPED_ENUM_VALUE(Value1)
    ADD_SCOPED_ENUM_VALUE(Value2)
END_EXPORT_ENUM(EEnumClass)

BEGIN_EXPORT_REFLECTED_CLASS(UUnLuaTestStub)
    ADD_PROPERTY(ScopedEnum)
    ADD_PROPERTY(EnumClass)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(UUnLuaTestStub)

#endif
int32 UUnLuaTestStub::TestForIssue407(TArray<int32> Array)
{
    return Array.Num();
}
