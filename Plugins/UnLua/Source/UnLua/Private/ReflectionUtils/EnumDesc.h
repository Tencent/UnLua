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

#include "UnLuaBase.h"
#include "CoreUObject.h"
#include "Engine/UserDefinedEnum.h"
#include "HAL/Platform.h"

/**
 * Enum descriptor
 */
class FEnumDesc
{
public:
    explicit FEnumDesc(UEnum* InEnum);

    FORCEINLINE bool IsValid() const { return Enum.IsValid(); }

    FORCEINLINE const FString& GetName() const { return EnumName; }

    FORCEINLINE int64 GetValue(const FName EntryName)
    {
        Load();

        if (!IsValid())
        {
            UE_LOG(LogUnLua, Warning, TEXT("%s:Invalid enum[%s] descriptor"), ANSI_TO_TCHAR(__FUNCTION__), *EnumName);
            return INDEX_NONE;
        }

        if (!bUserDefined)
            return Enum->GetValueByName(EntryName);

        for (auto i = 0; i < Enum->NumEnums(); ++i)
        {
            FName DisplayName(*Enum->GetDisplayNameTextByIndex(i).ToString());
            if (DisplayName == EntryName)
                return Enum->GetValueByIndex(i);
        }

        return INDEX_NONE;
    }

    FORCEINLINE UEnum* GetEnum()
    {
        Load();
        return Enum.Get();
    }

    void Load();

    void UnLoad();

private:
    TWeakObjectPtr<UEnum> Enum;

    FString EnumName;
    bool bUserDefined;
};
