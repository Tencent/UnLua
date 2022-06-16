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


#include "LuaFunction.h"
#include "UnLuaModule.h"
#include "ReflectionUtils/PropertyDesc.h"

static constexpr auto RenameFlags = REN_DontCreateRedirectors | REN_DoNotDirty | REN_ForceNoResetLoaders | REN_NonTransactional;
static auto OverriddenSuffix = FUTF8ToTCHAR("__Overridden");

TMap<UClass*, UClass*> ULuaFunction::SuspendedOverrides;

static UClass* MakeOrphanedClass(const UClass* Class)
{
    FString OrphanedClassString = FString::Printf(TEXT("ORPHANED_DATA_ONLY_%s"), *Class->GetName());
    FName OrphanedClassName = MakeUniqueObjectName(GetTransientPackage(), UBlueprintGeneratedClass::StaticClass(), FName(*OrphanedClassString));
    UClass* OrphanedClass = NewObject<UBlueprintGeneratedClass>(GetTransientPackage(), OrphanedClassName, RF_Public | RF_Transient);
    OrphanedClass->ClassAddReferencedObjects = Class->AddReferencedObjects;
    OrphanedClass->ClassFlags |= CLASS_CompiledFromBlueprint;
#if WITH_EDITOR
    OrphanedClass->ClassGeneratedBy = Class->ClassGeneratedBy;
#endif
    return OrphanedClass;
}

DEFINE_FUNCTION(ULuaFunction::execCallLua)
{
    const auto LuaFunction = Cast<ULuaFunction>(Stack.CurrentNativeFunction);
    const auto Env = IUnLuaModule::Get().GetEnv(Context);
    if (!Env)
    {
        // PIE 结束时可能已经没有Lua环境了
        return;
    }
    Env->GetFunctionRegistry()->Invoke(LuaFunction, Context, Stack, RESULT_PARAM);
}

bool ULuaFunction::IsOverridable(const UFunction* Function)
{
    static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
    static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
    return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

bool ULuaFunction::Override(UFunction* Function, UClass* Outer, FName NewName)
{
    ULuaFunction* LuaFunction;
    const auto bReplace = Function->GetOuter() == Outer;
    if (bReplace)
    {
        // ReplaceFunction
        LuaFunction = Cast<ULuaFunction>(Function);
        if (LuaFunction)
        {
            LuaFunction->Initialize();
            return true;
        }

        const auto OverriddenName = FString::Printf(TEXT("%s%s"), *Function->GetName(), OverriddenSuffix.Get());
        Function->Rename(*OverriddenName, nullptr, RenameFlags);
        Outer->AddFunctionToFunctionMap(Function, Function->GetFName());
    }
    else
    {
        // AddFunction
        if (Outer->FindFunctionByName(NewName, EIncludeSuperFlag::ExcludeSuper))
            return false;

        if (Function->HasAnyFunctionFlags(FUNC_Native))
        {
            // Need to do this before the call to DuplicateObject in the case that the super-function already has FUNC_Native
            Outer->AddNativeFunction(*NewName.ToString(), &execCallLua);
        }
    }

    FObjectDuplicationParameters DuplicationParams(Function, Outer);
    DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
    DuplicationParams.DestName = NewName;
    DuplicationParams.DestClass = StaticClass();
    LuaFunction = static_cast<ULuaFunction*>(StaticDuplicateObjectEx(DuplicationParams));
    LuaFunction->FunctionFlags |= FUNC_Native;
    LuaFunction->Overridden = Function;
    LuaFunction->ClearInternalFlags(EInternalObjectFlags::Native);
    LuaFunction->SetNativeFunc(execCallLua);

    if (bReplace)
        LuaFunction->SetSuperStruct(Function->GetSuperStruct());
    else
        LuaFunction->SetSuperStruct(Function);

    if (!FPlatformProperties::RequiresCookedData())
        UMetaData::CopyMetadata(Function, LuaFunction);

    LuaFunction->StaticLink(true);
    LuaFunction->Initialize();

    Outer->AddFunctionToFunctionMap(LuaFunction, NewName);

    if (Outer->IsRooted() || GUObjectArray.IsDisregardForGC(Outer))
    {
        LuaFunction->AddToRoot();
    }
    else
    {
        LuaFunction->Next = Outer->Children;
        Outer->Children = LuaFunction;
        LuaFunction->AddToCluster(Outer);
    }

    return true;
}

void ULuaFunction::RestoreOverrides(UClass* Class)
{
    auto OrphanedClass = MakeOrphanedClass(Class);
    auto Current = &Class->Children;
    while (*Current)
    {
        auto LuaFunction = Cast<ULuaFunction>(*Current);
        if (!LuaFunction)
        {
            Current = &(*Current)->Next;
            continue;
        }

        *Current = LuaFunction->Next;
        const auto Overridden = LuaFunction->GetOverridden();
        if (Overridden && Overridden->GetOuter() == Class)
        {
            check(Overridden->GetName().EndsWith(OverriddenSuffix.Get()));
            Class->RemoveFunctionFromFunctionMap(Overridden);
            LuaFunction->Rename(nullptr, OrphanedClass, RenameFlags);
            FLinkerLoad::InvalidateExport(LuaFunction);
            Overridden->Rename(*Overridden->GetName().LeftChop(OverriddenSuffix.Length()), nullptr, RenameFlags);
            Class->AddFunctionToFunctionMap(Overridden, Overridden->GetFName());
        }
    }
    Class->ClearFunctionMapsCaches();
}

void ULuaFunction::SuspendOverrides(UClass* Class)
{
    check(!SuspendedOverrides.Contains(Class));
    auto OrphanedClass = MakeOrphanedClass(Class);
    SuspendedOverrides.Add(Class, OrphanedClass);

    auto Current = &Class->Children;
    while (*Current)
    {
        auto LuaFunction = Cast<ULuaFunction>(*Current);
        if (!LuaFunction)
        {
            Current = &(*Current)->Next;
            continue;
        }

        *Current = LuaFunction->Next;
        const auto Overridden = LuaFunction->GetOverridden();
        if (!Overridden || Overridden->GetOuter() != Class)
            continue;

        LuaFunction->Rename(nullptr, OrphanedClass, RenameFlags);
        Overridden->Rename(*Overridden->GetName().LeftChop(OverriddenSuffix.Length()), nullptr, RenameFlags);

        const auto OrphanedNext = OrphanedClass->Children;
        OrphanedClass->Children = LuaFunction;
        LuaFunction->Next = OrphanedNext;
    }
}

void ULuaFunction::ResumeOverrides(UClass* Class)
{
    auto& Origin = *Class;
    auto& Orphaned = *SuspendedOverrides.FindAndRemoveChecked(Class);

    auto Current = Orphaned.Children;
    while (Current)
    {
        auto LuaFunction = Cast<ULuaFunction>(Current);
        if (!LuaFunction)
        {
            Current = Current->Next;
            continue;
        }

        const auto Next = Current->Next;
        Current->Next = Origin.Children;
        Origin.Children = Current;
        Current = Next;

        const auto Overridden = LuaFunction->GetOverridden();
        if (!Overridden)
            continue;

        const auto Name = Overridden->GetName();
        Overridden->Rename(*(Name + OverriddenSuffix.Get()), nullptr, RenameFlags);
        LuaFunction->Rename(nullptr, Overridden->GetOuter(), RenameFlags);
    }
}

void ULuaFunction::GetOverridableFunctions(UClass* Class, TMap<FName, UFunction*>& Functions)
{
    if (!Class)
        return;

    // all 'BlueprintEvent'
    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::IncludeInterfaces); It; ++It)
    {
        UFunction* Function = *It;
        if (!IsOverridable(Function))
            continue;
        FName FuncName = Function->GetFName();
        UFunction** FuncPtr = Functions.Find(FuncName);
        if (!FuncPtr)
            Functions.Add(FuncName, Function);
    }

    // all 'RepNotifyFunc'
    for (int32 i = 0; i < Class->ClassReps.Num(); ++i)
    {
        FProperty* Property = Class->ClassReps[i].Property;
        if (!Property->HasAnyPropertyFlags(CPF_RepNotify))
            continue;
        UFunction* Function = Class->FindFunctionByName(Property->RepNotifyFunc);
        if (!Function)
            continue;
        UFunction** FuncPtr = Functions.Find(Property->RepNotifyFunc);
        if (!FuncPtr)
            Functions.Add(Property->RepNotifyFunc, Function);
    }
}

void ULuaFunction::Initialize()
{
    Desc = MakeShared<FFunctionDesc>(this, nullptr);
}

UFunction* ULuaFunction::GetOverridden() const
{
    return Overridden.Get();
}

#if WITH_EDITOR
void ULuaFunction::Bind()
{
    const auto Outer = GetOuter();
    if (Outer && Outer->GetName().StartsWith("REINST_"))
    {
        FunctionFlags &= ~FUNC_Native;
        return;
    }
    Super::Bind();
}

#endif
