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
    auto ClassNameString = FString::Printf(TEXT("LUA_OVERRIDES_%s"), *Class->GetName());
    auto ClassName = MakeUniqueObjectName(GetTransientPackage(), Class, FName(*ClassNameString));
    auto Ret = NewObject<ULuaOverridesClass>(GetTransientPackage(), ClassName, RF_Public | RF_Transient);
    Ret->ClassFlags |= CLASS_NewerVersionExists; // bypass FBlueprintActionDatabase::RefreshClassActions
    Ret->ClassDefaultObject = StaticClass()->GetDefaultObject();
    Ret->SetSuperStruct(StaticClass());
    Ret->Bind();
    Ret->Owner = Class;
    Ret->AddToOwner();
    return Ret;
}

void ULuaOverridesClass::Restore()
{
    SetActive(false);
    RemoveFromOwner();
}

void ULuaOverridesClass::SetActive(const bool bActive)
{
    const auto Class = Owner.Get();
    if (!Class)
        return;

    for (TFieldIterator<ULuaFunction> It(this, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        const auto LuaFunction = *It;
        LuaFunction->SetActive(bActive);
    }

    Class->ClearFunctionMapsCaches();
}

void ULuaOverridesClass::BeginDestroy()
{
    Restore();
    UClass::BeginDestroy();
}

void ULuaOverridesClass::AddToOwner()
{
    const auto Class = Owner.Get();
    if (!Class)
        return;

    auto Field = &Class->Children;
    while (*Field)
        Field = &(*Field)->Next;
    *Field = this;

    if (Class->IsRooted() || GUObjectArray.IsDisregardForGC(Class))
        AddToRoot();
}

void ULuaOverridesClass::RemoveFromOwner()
{
    const auto Class = Owner.Get();
    if (!Class)
        return;

    auto Field = &Class->Children;
    while (*Field)
    {
        if (*Field == this)
        {
            *Field = nullptr;
            break;
        }
        Field = &(*Field)->Next;
    }

    if (!Class->IsRooted() && !GUObjectArray.IsDisregardForGC(Class))
        RemoveFromRoot();
}
