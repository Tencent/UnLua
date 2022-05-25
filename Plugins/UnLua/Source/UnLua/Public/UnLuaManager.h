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

#include "InputCoreTypes.h"
#include "lauxlib.h"
#include "UnLuaCompatibility.h"
#include "UnLuaManager.generated.h"

namespace UnLua
{
    class FLuaEnv;
}

UCLASS()
class UNLUA_API UUnLuaManager : public UObject
{
    GENERATED_BODY()

public:

    UnLua::FLuaEnv* Env;

    UUnLuaManager();

    bool Bind(UObject *Object, const TCHAR *InModuleName, int32 InitializerTableRef = LUA_NOREF);

    void OnWorldCleanup(UWorld* World, bool bArg, bool bCond);
    
    void NotifyUObjectDeleted(const UObjectBase *Object);

    void Cleanup();

    void CleanUpByClass(UClass *Class);

    void GetDefaultInputs();

    void CleanupDefaultInputs();

    bool ReplaceInputs(AActor *Actor, class UInputComponent *InputComponent);

    void OnMapLoaded(UWorld *World);

    UFUNCTION()
    void OnLatentActionCompleted(int32 LinkID);

    UFUNCTION(BlueprintImplementableEvent)
    void InputAction(FKey Key);

    UFUNCTION(BlueprintImplementableEvent)
    void InputAxis(float AxisValue);

    UFUNCTION(BlueprintImplementableEvent)
    void InputTouch(ETouchIndex::Type FingerIndex, const FVector &Location);

    UFUNCTION(BlueprintImplementableEvent)
    void InputVectorAxis(const FVector &AxisValue);

    UFUNCTION(BlueprintImplementableEvent)
    void InputGesture(float Value);

    UFUNCTION(BlueprintImplementableEvent)
    void TriggerAnimNotify();

private:
    UClass* GetTargetClass(UClass *Class, UFunction **GetModuleNameFunc = nullptr);

    bool BindInternal(UClass *Class, const FString &InModuleName, bool bMultipleLuaBind, FString &Error);
    bool ConditionalUpdateClass(UClass *Class, const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions);

    void OverrideFunctions(const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions, UClass *OuterClass);

    void ReplaceActionInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceTouchInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceVectorAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceGestureInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);

    TMap<UClass*, FString> ModuleNames;
    TMap<FString, int16> RealModuleNames;
    TMap<FString, UClass*> Classes;
    TMap<FString, TSet<FName>> ModuleFunctions;

    TSet<FName> DefaultAxisNames;
    TSet<FName> DefaultActionNames;
    TArray<FKey> AllKeys;

    UFunction *InputActionFunc;
    UFunction *InputAxisFunc;
    UFunction *InputTouchFunc;
    UFunction *InputVectorAxisFunc;
    UFunction *InputGestureFunc;
    UFunction *AnimNotifyFunc;
};
