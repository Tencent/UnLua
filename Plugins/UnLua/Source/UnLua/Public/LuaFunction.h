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
#include "LuaFunction.generated.h"

namespace UnLua
{
    class FLuaEnv;
}

class FFunctionDesc;

UCLASS()
class UNLUA_API ULuaFunction : public UFunction
{
    GENERATED_BODY()

public:
    /**
    * Whether the UFunction is overridable
    */
    static bool IsOverridable(const UFunction* Function);

    static bool Override(UFunction* Function, UClass* Outer, FName NewName);

    /**
     * 还原指定UClass上所有覆写的UFunction。
     * 在退出PIE时使用。
     * @return 返回一个临时UClass，其中包含了所有被抽出来的ULuaFunction
     */
    static void RestoreOverrides(UClass* Class);

    /**
     * 临时挂起指定UClass上所有覆写的ULuaFunction，用于后续恢复。
     * 在PIE保存UPackage时使用，避免ULuaFunction随着UClass被序列化到本地。
     */
    static void SuspendOverrides(UClass* Class);

    /**
     * 恢复指定UClass临时挂起的ULuaFunction。
     * 在PIE保存UPackage完成后使用。
     */
    static void ResumeOverrides(UClass* Class);
    
    /*
     * Get all UFUNCTION that can be overrode
     */
    static void GetOverridableFunctions(UClass* Class, TMap<FName, UFunction*>& Functions);

    /**
     * Custom thunk function to call Lua function
     */
    DECLARE_FUNCTION(execCallLua);

    void Initialize();

    UFunction* GetOverridden() const;

#if WITH_EDITOR
    virtual void Bind() override;
#endif

private:
    TWeakObjectPtr<UFunction> Overridden;
    TSharedPtr<FFunctionDesc> Desc;
    static TMap<UClass*, UClass*> SuspendedOverrides;
};
