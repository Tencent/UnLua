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

#include "LuaOverridesClass.h"
#include "LuaFunction.h"

ULuaOverridesClass* ULuaOverridesClass::Create(UClass* Class)
{
    auto ClassName = FString::Printf(TEXT("LUA_OVERRIDES_%s"), *Class->GetName());
    auto Ret = NewObject<ULuaOverridesClass>(GetTransientPackage(), *ClassName, RF_Public | RF_Transient);
    Ret->AddToRoot();
    Ret->Owner = Class;
    return Ret;
}

void ULuaOverridesClass::Restore()
{
    RemoveFromRoot();
    const auto Class = Owner.Get();
    if (!Class)
        return;

    for (TFieldIterator<ULuaFunction> It(this, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        auto LuaFunction = *It;
        LuaFunction->Restore();
    }
    Class->ClearFunctionMapsCaches();
}
