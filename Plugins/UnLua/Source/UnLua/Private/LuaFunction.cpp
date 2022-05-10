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

        const auto OverriddenName = FString::Printf(TEXT("%s%s"), *Function->GetName(), TEXT("__Overridden"));
        constexpr auto RenameFlags = REN_DontCreateRedirectors | REN_DoNotDirty | REN_ForceNoResetLoaders | REN_NonTransactional;
        Function->Rename(*OverriddenName, Function->GetOuter(), RenameFlags);
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
    LuaFunction = Cast<ULuaFunction>(StaticDuplicateObjectEx(DuplicationParams));
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
