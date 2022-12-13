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
#include "Engine/DynamicBlueprintBinding.h"
#include "lua.hpp"
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

    void NotifyUObjectDeleted(const UObjectBase *Object);

    void Cleanup();

    int GetBoundRef(const UClass* Class);

    void GetDefaultInputs();

    void CleanupDefaultInputs();

    bool ReplaceInputs(AActor *Actor, class UInputComponent *InputComponent);

    void OnMapLoaded(UWorld *World);

    UFUNCTION(BlueprintCallable)
    UDynamicBlueprintBinding* GetOrAddBindingObject(UClass* Class, UClass* BindingClass);

    UFUNCTION(BlueprintCallable)
    void Override(UClass* Class, FName FunctionName, FName LuaFunctionName);

    UFUNCTION()
    void OnLatentActionCompleted(int32 LinkID);

    UFUNCTION(BlueprintImplementableEvent)
    void EnhancedInputActionDigital(bool ActionValue, float ElapsedSeconds, float TriggeredSeconds);

    UFUNCTION(BlueprintImplementableEvent)
    void EnhancedInputActionAxis1D(float ActionValue, float ElapsedSeconds, float TriggeredSeconds);

    UFUNCTION(BlueprintImplementableEvent)
    void EnhancedInputActionAxis2D(const FVector2D& ActionValue, float ElapsedSeconds, float TriggeredSeconds);

    UFUNCTION(BlueprintImplementableEvent)
    void EnhancedInputActionAxis3D(const FVector& ActionValue, float ElapsedSeconds, float TriggeredSeconds);
    
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
    /* 将一个UClass绑定到Lua模块，根据这个模块定义的函数列表来覆盖上面的UFunction */
    bool BindClass(UClass *Class, const FString &InModuleName, FString &Error);

    void ReplaceActionInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceTouchInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceAxisKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceVectorAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);
    void ReplaceGestureInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions);

    struct FClassBindInfo
    {
        UClass* Class;
        FString ModuleName;
        int TableRef;
        TSet<FName> LuaFunctions;
        TMap<FName, UFunction*> UEFunctions;
    };

    TMap<UClass*, FClassBindInfo> Classes;

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
