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

#include "CoreMinimal.h"
#include "LuaOverridesClass.generated.h"

/** 用于承载所有运行时覆写创建的ULuaFunction */
UCLASS(Transient)
class UNLUA_API ULuaOverridesClass : public UClass
{
    GENERATED_BODY()

public:
    static ULuaOverridesClass* Create(UClass* Class);

    void Restore();

    void SetActive(const bool bActive);

    FORCEINLINE UClass* GetOwner() const { return Owner.Get(); }
    
    virtual void BeginDestroy() override;

private:
    void AddToOwner();

    void RemoveFromOwner();

    TWeakObjectPtr<UClass> Owner;
};
