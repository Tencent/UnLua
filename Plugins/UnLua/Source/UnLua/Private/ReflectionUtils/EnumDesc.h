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
#include "Engine/UserDefinedEnum.h"
#include "LuaContext.h"

/**
 * Enum descriptor
 */
class FEnumDesc
{
public:
    enum class EType
    {
        Enum,
        UserDefinedEnum,
    };

    explicit FEnumDesc(UEnum *InEnum, EType EType = EType::Enum);

    ~FEnumDesc();

    FORCEINLINE bool IsValid() const { return Enum && GLuaCxt->IsUObjectValid(Enum); }

    FORCEINLINE const FString& GetName() const { return EnumName; }

    template <typename CharType>
    FORCEINLINE int64 GetValue(const CharType *EntryName) const
    {
        static int64 (*Func[2])(UEnum*, FName) = { FEnumDesc::GetEnumValue, FEnumDesc::GetUserDefinedEnumValue };
        return (Func[(int32)Type])(Enum, FName(EntryName));
    }

    FORCEINLINE UEnum* GetEnum() const { return Enum; }

    static int64 GetEnumValue(UEnum *Enum, FName EntryName)
    {
        check(Enum);
        return Enum->GetValueByName(EntryName);
    }

    static int64 GetUserDefinedEnumValue(UEnum *Enum, FName EntryName)
    {
        check(Enum);
        int32 NumEntries = Enum->NumEnums();
        for (int32 i = 0; i < NumEntries; ++i)
        {
            FName DisplayName(*Enum->GetDisplayNameTextByIndex(i).ToString());
            if (DisplayName == EntryName)
            {
                return Enum->GetValueByIndex(i);
            }
        }
        return INDEX_NONE;
    }

private:
    union
    {
        UEnum *Enum;
        UUserDefinedEnum *UserDefinedEnum;
    };

    FString EnumName;
    EType Type;
};
