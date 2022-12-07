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
#include "LuaOverrides.h"
#include "LuaOverridesClass.h"
#include "UnLuaModule.h"
#include "ReflectionUtils/PropertyDesc.h"

static constexpr uint8 ScriptMagicHeader[] = {EX_StringConst, 'L', 'U', 'A', '\0', EX_UInt64Const};
static constexpr size_t ScriptMagicHeaderSize = sizeof ScriptMagicHeader;

TMap<UClass*, UClass*> ULuaFunction::SuspendedOverrides;

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

DEFINE_FUNCTION(ULuaFunction::execScriptCallLua)
{
    const auto LuaFunction = Get(Stack.CurrentNativeFunction);
    if (!LuaFunction)
        return;
    const auto Env = IUnLuaModule::Get().GetEnv(Context);
    if (!Env)
    {
        // PIE 结束时可能已经没有Lua环境了
        return;
    }
    Env->GetFunctionRegistry()->Invoke(LuaFunction, Context, Stack, RESULT_PARAM);
}

ULuaFunction* ULuaFunction::Get(UFunction* Function)
{
    if (!Function)
        return nullptr;

    const auto LuaFunction = Cast<ULuaFunction>(Function);
    if (LuaFunction)
        return LuaFunction;

    if (Function->Script.Num() < ScriptMagicHeaderSize + sizeof(ULuaFunction*))
        return nullptr;

    const auto Data = Function->Script.GetData();
    if (!Data)
        return nullptr;

    if (FPlatformMemory::Memcmp(Data, ScriptMagicHeader, ScriptMagicHeaderSize) != 0)
        return nullptr;

    return FPlatformMemory::ReadUnaligned<ULuaFunction*>(Data + ScriptMagicHeaderSize);
}

bool ULuaFunction::IsOverridable(const UFunction* Function)
{
    static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
    static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
    return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

bool ULuaFunction::Override(UFunction* Function, UClass* Outer, FName NewName)
{
    UnLua::FLuaOverrides::Get().Override(Function, Outer, NewName);
    return true;
}

void ULuaFunction::RestoreOverrides(UClass* Class)
{
    UnLua::FLuaOverrides::Get().Restore(Class);
}

void ULuaFunction::SuspendOverrides(UClass* Class)
{
    // check(!SuspendedOverrides.Contains(Class));
    // auto OrphanedClass = MakeOrphanedClass(Class);
    // SuspendedOverrides.Add(Class, OrphanedClass);
    //
    // auto Current = &Class->Children;
    // while (*Current)
    // {
    //     auto LuaFunction = Cast<ULuaFunction>(*Current);
    //     if (!LuaFunction)
    //     {
    //         Current = &(*Current)->Next;
    //         continue;
    //     }
    //
    //     *Current = LuaFunction->Next;
    //     const auto Overridden = LuaFunction->GetOverridden();
    //     if (!Overridden || Overridden->GetOuter() != Class)
    //         continue;
    //
    //     LuaFunction->Rename(nullptr, OrphanedClass, RenameFlags);
    //     Overridden->Rename(*Overridden->GetName().LeftChop(OverriddenSuffix.Length()), nullptr, RenameFlags);
    //
    //     const auto OrphanedNext = OrphanedClass->Children;
    //     OrphanedClass->Children = LuaFunction;
    //     LuaFunction->Next = OrphanedNext;
    // }
}

void ULuaFunction::ResumeOverrides(UClass* Class)
{
    // auto& Origin = *Class;
    // auto& Orphaned = *SuspendedOverrides.FindAndRemoveChecked(Class);
    //
    // auto Current = Orphaned.Children;
    // while (Current)
    // {
    //     auto LuaFunction = Cast<ULuaFunction>(Current);
    //     if (!LuaFunction)
    //     {
    //         Current = Current->Next;
    //         continue;
    //     }
    //
    //     const auto Next = Current->Next;
    //     Current->Next = Origin.Children;
    //     Origin.Children = Current;
    //     Current = Next;
    //
    //     const auto Overridden = LuaFunction->GetOverridden();
    //     if (!Overridden)
    //         continue;
    //
    //     const auto Name = Overridden->GetName();
    //     Overridden->Rename(*(Name + OverriddenSuffix.Get()), nullptr, RenameFlags);
    //     LuaFunction->Rename(nullptr, Overridden->GetOuter(), RenameFlags);
    // }
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

void ULuaFunction::Override(UFunction* Function, UClass* Class, bool bAddNew)
{
    check(Function && Class);

#if WITH_METADATA
    UMetaData::CopyMetadata(Function, this);
#endif

    bAdded = bAddNew;
    Overridden = Function;
    OverriddenNativeFunc = Function->GetNativeFunc();
    OverriddenFlags = Function->FunctionFlags;

    if (bAddNew)
    {
        check(!Class->FindFunctionByName(GetFName(), EIncludeSuperFlag::ExcludeSuper));
        SetSuperStruct(Function);
        FunctionFlags |= FUNC_Native;
        ClearInternalFlags(EInternalObjectFlags::Native);
        SetNativeFunc(execCallLua);

        Class->AddFunctionToFunctionMap(this, *GetName());
        if (Function->HasAnyFunctionFlags(FUNC_Native))
            Class->AddNativeFunction(*GetName(), &ULuaFunction::execCallLua);
    }
    else
    {
        SetSuperStruct(Function->GetSuperStruct());
        Script = Function->Script;

        Function->FunctionFlags |= FUNC_Native;
        Function->SetNativeFunc(&execScriptCallLua);
        Function->GetOuterUClass()->AddNativeFunction(*Function->GetName(), &execScriptCallLua);
        Function->Script.Empty();
        Function->Script.AddUninitialized(ScriptMagicHeaderSize + sizeof(ULuaFunction*));
        const auto Data = Function->Script.GetData();
        FPlatformMemory::Memcpy(Data, ScriptMagicHeader, ScriptMagicHeaderSize);
        FPlatformMemory::WriteUnaligned<ULuaFunction*>(Data + ScriptMagicHeaderSize, this);
    }
}

void ULuaFunction::Restore()
{
    if (bAdded)
    {
        const auto OverriddenClass = Cast<ULuaOverridesClass>(GetOuter())->Owner;
        OverriddenClass->RemoveFunctionFromFunctionMap(this);
    }
    else
    {
        const auto Old = Overridden.Get();
        if (!Old)
            return;
        Old->Script = Script;
        Old->SetNativeFunc(OverriddenNativeFunc);
        Old->GetOuterUClass()->AddNativeFunction(*Old->GetName(), OverriddenNativeFunc);
        Old->FunctionFlags = OverriddenFlags;
    }
    Overridden.Reset();
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
