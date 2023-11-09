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
#include "Misc/EngineVersionComparison.h"

static constexpr uint8 ScriptMagicHeader[] = {EX_StringConst, 'L', 'U', 'A', '\0', EX_UInt64Const};
static constexpr size_t ScriptMagicHeaderSize = sizeof ScriptMagicHeader;

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
    UnLua::FLuaOverrides::Get().Suspend(Class);
}

void ULuaFunction::ResumeOverrides(UClass* Class)
{
    UnLua::FLuaOverrides::Get().Resume(Class);
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
    check(Function && Class && !From.IsValid());

#if WITH_METADATA
    UMetaData::CopyMetadata(Function, this);
#endif

    bActivated = false;
    bAdded = bAddNew;
    From = Function;

    if (Function->GetNativeFunc() == execScriptCallLua)
    {
        // 目标UFunction可能已经被覆写过了
        const auto LuaFunction = Get(Function);
        Overridden = LuaFunction->GetOverridden();
        check(Overridden);
    }
    else
    {
        const auto DestName = FString::Printf(TEXT("%s__Overridden"), *Function->GetName());
        if (Function->HasAnyFunctionFlags(FUNC_Native))
            GetOuterUClass()->AddNativeFunction(*DestName, *Function->GetNativeFunc());
        Overridden = static_cast<UFunction*>(StaticDuplicateObject(Function, GetOuter(), *DestName));
        Overridden->ClearInternalFlags(EInternalObjectFlags::Native);
        Overridden->StaticLink(true);
        Overridden->SetNativeFunc(Function->GetNativeFunc());
    }

    SetActive(true);
}

void ULuaFunction::Restore()
{
    if (bAdded)
    {
        if (const auto OverriddenClass = Cast<ULuaOverridesClass>(GetOuter())->GetOwner())
            OverriddenClass->RemoveFunctionFromFunctionMap(this);
    }
    else
    {
        const auto Old = From.Get();
        if (!Old)
            return;
        Old->Script = Script;
        Old->SetNativeFunc(Overridden->GetNativeFunc());
        Old->GetOuterUClass()->AddNativeFunction(*Old->GetName(), Overridden->GetNativeFunc());
        Old->FunctionFlags = Overridden->FunctionFlags;
    }
}

UClass* ULuaFunction::GetOverriddenUClass() const
{
    const auto OverridesClass = Cast<ULuaOverridesClass>(GetOuter());
    return OverridesClass ? OverridesClass->GetOwner() : nullptr;
}

void ULuaFunction::SetActive(const bool bActive)
{
    if (bActivated == bActive)
        return;

    const auto Function = From.Get();
    if (!Function)
        return;

    const auto Class = Cast<ULuaOverridesClass>(GetOuter())->GetOwner();
    if (!Class)
        return;
    
    if (bActive)
    {
        if (bAdded)
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
            Children = Function->Children;
            ChildProperties = Function->ChildProperties;
            PropertyLink = Function->PropertyLink;

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
    else
    {
        if (bAdded)
        {
            Class->RemoveFunctionFromFunctionMap(this);
        }
        else
        {
            Children = nullptr;
            ChildProperties = nullptr;

            Function->Script = Script;
            Function->SetNativeFunc(Overridden->GetNativeFunc());
            Function->GetOuterUClass()->AddNativeFunction(*Function->GetName(), Overridden->GetNativeFunc());
            Function->FunctionFlags = Overridden->FunctionFlags;
        }
    }
    
    bActivated = bActive;
}

void ULuaFunction::FinishDestroy()
{
    if (bActivated && !bAdded)
    {
        Children = nullptr;
        ChildProperties = nullptr;
    }
    UFunction::FinishDestroy();
}

UFunction* ULuaFunction::GetOverridden() const
{
    return Overridden;
}

void ULuaFunction::Bind()
{
    if (From.IsValid())
    {
        if (bAdded)
            SetNativeFunc(execCallLua);
        else
            SetNativeFunc(Overridden->GetNativeFunc());
    }
    else
    {
#if UE_VERSION_NEWER_THAN(5, 2, 1)
        Super::Bind();
#else
        SetNativeFunc(ProcessInternal);
#endif
    }
}
