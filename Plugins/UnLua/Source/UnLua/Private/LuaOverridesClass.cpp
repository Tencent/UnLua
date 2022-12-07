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

#include "Misc/EngineVersionComparison.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "LuaOverridesClass.h"
#include "LuaFunction.h"

static UClass* MakeOrphanedClass(const UClass* Class)
{
    FString OrphanedClassString = FString::Printf(TEXT("ORPHANED_DATA_ONLY_%s"), *Class->GetName());
    FName OrphanedClassName = MakeUniqueObjectName(GetTransientPackage(), UBlueprintGeneratedClass::StaticClass(), FName(*OrphanedClassString));
    UClass* OrphanedClass = NewObject<UBlueprintGeneratedClass>(GetTransientPackage(), OrphanedClassName, RF_Public | RF_Transient);
#if UE_VERSION_OLDER_THAN(5, 1, 0)
    OrphanedClass->ClassAddReferencedObjects = Class->AddReferencedObjects;
#else
    OrphanedClass->CppClassStaticFunctions = Class->CppClassStaticFunctions;
#endif
    OrphanedClass->ClassFlags |= CLASS_CompiledFromBlueprint;
#if WITH_EDITOR
    OrphanedClass->ClassGeneratedBy = Class->ClassGeneratedBy;
#endif
    return OrphanedClass;
}

void ULuaOverridesClass::Restore()
{
    const auto Class = Owner.Get();
    if (!Class)
        return;

    // auto OrphanedClass = MakeOrphanedClass(Class);
    for (TFieldIterator<ULuaFunction> It(this, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        auto LuaFunction = *It;
        LuaFunction->Restore();
    }
    Class->ClearFunctionMapsCaches();
}
