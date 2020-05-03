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
#include "Engine/EngineBaseTypes.h"
#include "UnLuaCompatibility.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "UnLuaManager.generated.h"

UCLASS()
class UUnLuaManager : public UObject
{
    GENERATED_BODY()

public:
    UUnLuaManager();

    bool Bind(UObjectBaseUtility *Object, UClass *Class, const TCHAR *InModuleName, int32 InitializerTableRef = INDEX_NONE);

    bool OnModuleHotfixed(const TCHAR *InModuleName);

    void NotifyUObjectDeleted(const UObjectBase *Object, bool bClass = false);

    void Cleanup(class UWorld *World, bool bFullCleanup);

    void CleanUpByClass(UClass *Class);

    void PostCleanup();

    void GetDefaultInputs();

    void CleanupDefaultInputs();

    bool ReplaceInputs(AActor *Actor, class UInputComponent *InputComponent);

    void OnMapLoaded(UWorld *World);

    void OnActorSpawned(class AActor *Actor);

    UFUNCTION()
    void OnActorDestroyed(class AActor *Actor);

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
    void OnDerivedClassBinded(UClass *DerivedClass, UClass *BaseClass);

    UClass* GetTargetClass(UClass *Class, UFunction **GetModuleNameFunc = nullptr);

    bool BindInternal(UObjectBaseUtility *Object, UClass *Class, const FString &InModuleName, bool bNewCreated);
    bool BindSurvivalObject(struct lua_State *L, UObjectBaseUtility *Object, UClass *Class, const char *ModuleName);
    bool ConditionalUpdateClass(UClass *Class, const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions);

    void OverrideFunctions(const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions, UClass *OuterClass, bool bCheckFuncNetMode = false);
    void OverrideFunction(UFunction *TemplateFunction, UClass *OuterClass, FName NewFuncName);
    void AddFunction(UFunction *TemplateFunction, UClass *OuterClass, FName NewFuncName);
    void ReplaceFunction(UFunction *TemplateFunction, UClass *OuterClass);

    void ReplaceActionInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceTouchInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceVectorAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceGestureInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);

    void AddAttachedObject(UObjectBaseUtility *Object, int32 ObjectRef);

    void CleanupDuplicatedFunctions();
    void CleanupCachedNatives();
    void CleanupCachedScripts();

    void OnClassCleanup(UClass *Class);
    void ResetUFunction(UFunction *Function, FNativeFuncPtr NativeFuncPtr);
    void RemoveDuplicatedFunctions(UClass *Class, TArray<UFunction*> &Functions);

    TMap<UClass*, FString> ModuleNames;
    TMap<FString, UClass*> Classes;
    TMap<UClass*, TMap<FName, UFunction*>> OverridableFunctions;
    TMap<UClass*, TArray<UFunction*>> DuplicatedFunctions;
    TMap<FString, TSet<FName>> ModuleFunctions;
    TMap<UFunction*, FNativeFuncPtr> CachedNatives;
    TMap<UFunction*, TArray<uint8>> CachedScripts;

#if !ENABLE_CALL_OVERRIDDEN_FUNCTION
    TMap<UFunction*, UFunction*> New2TemplateFunctions;
#endif

    TMap<UClass*, TArray<UClass*>> Base2DerivedClasses;
    TMap<UClass*, UClass*> Derived2BaseClasses;

    TSet<FName> DefaultAxisNames;
    TSet<FName> DefaultActionNames;
    TArray<FKey> AllKeys;

    TMap<UObjectBaseUtility*, int32> AttachedObjects;
    TSet<AActor*> AttachedActors;

    UFunction *InputActionFunc;
    UFunction *InputAxisFunc;
    UFunction *InputTouchFunc;
    UFunction *InputVectorAxisFunc;
    UFunction *InputGestureFunc;
    UFunction *AnimNotifyFunc;
};
