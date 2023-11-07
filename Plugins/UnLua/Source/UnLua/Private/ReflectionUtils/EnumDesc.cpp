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

#include "UnLuaCompatibility.h"
#include "EnumDesc.h"
#include "LowLevel.h"

FEnumDesc::FEnumDesc(UEnum* InEnum)
    : Enum(InEnum)
{
    check(Enum.IsValid());
    EnumName = UnLua::LowLevel::GetMetatableName(InEnum);
    bUserDefined = InEnum->IsA<UUserDefinedEnum>();
}

void FEnumDesc::Load()
{
    if (Enum.IsValid())
        return;

    Enum = FindFirstObject<UEnum>(*EnumName);
    if (!Enum.IsValid())
        Enum = LoadObject<UEnum>(nullptr, *EnumName);

    check(Enum.IsValid());
}

void FEnumDesc::UnLoad()
{
    if (!Enum.IsValid())
        return;

    Enum.Reset();
    Enum = nullptr;
}
