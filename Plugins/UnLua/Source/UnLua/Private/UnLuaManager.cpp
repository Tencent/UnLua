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

#include "GameFramework/InputSettings.h"
#include "Components/InputComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/LevelScriptActor.h"
#include "UnLuaManager.h"
#include "LuaEnv.h"
#include "UnLuaLegacy.h"
#include "UnLuaInterface.h"
#include "LuaCore.h"
#include "LuaFunction.h"
#include "ObjectReferencer.h"


static const TCHAR* SReadableInputEvent[] = { TEXT("Pressed"), TEXT("Released"), TEXT("Repeat"), TEXT("DoubleClick"), TEXT("Axis"), TEXT("Max") };

UUnLuaManager::UUnLuaManager()
    : InputActionFunc(nullptr), InputAxisFunc(nullptr), InputTouchFunc(nullptr), InputVectorAxisFunc(nullptr), InputGestureFunc(nullptr), AnimNotifyFunc(nullptr)
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

    GetDefaultInputs();             // get all Axis/Action inputs
    EKeys::GetAllKeys(AllKeys);     // get all key inputs

    // get template input UFunctions for InputAction/InputAxis/InputTouch/InputVectorAxis/InputGesture/AnimNotify
    UClass *Class = GetClass();
    InputActionFunc = Class->FindFunctionByName(FName("InputAction"));
    InputAxisFunc = Class->FindFunctionByName(FName("InputAxis"));
    InputTouchFunc = Class->FindFunctionByName(FName("InputTouch"));
    InputVectorAxisFunc = Class->FindFunctionByName(FName("InputVectorAxis"));
    InputGestureFunc = Class->FindFunctionByName(FName("InputGesture"));
    AnimNotifyFunc = Class->FindFunctionByName(FName("TriggerAnimNotify"));

    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UUnLuaManager::OnWorldCleanup);
}

/**
 * Bind a Lua module for a UObject
 */
bool UUnLuaManager::Bind(UObject *Object, const TCHAR *InModuleName, int32 InitializerTableRef)
{   
    if (!Object)
    {
        UE_LOG(LogUnLua, Warning, TEXT("Invalid target object!"));
        return false;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("UUnLuaManager::Bind : %p,%s,%s"), Object, *Object->GetName(),InModuleName);
#endif

    UClass* Class = Object->GetClass();
    lua_State *L = Env->GetMainState();

    bool bMultipleLuaBind = false;
    UClass** BindedClass = Classes.Find(InModuleName);
    if ((BindedClass)
        &&(*BindedClass != Object->GetClass()))
    {
        bMultipleLuaBind = true;
    }

    if (!Env->GetClassRegistry()->Register(Class))
        return false;

    // try bind lua if not bind or use a copyed table
    UnLua::FLuaRetValues RetValues = UnLua::Call(L, "require", TCHAR_TO_UTF8(InModuleName));    // require Lua module
    FString Error;
    bool bSuccess;
    if (!RetValues.IsValid() || RetValues.Num() == 0)
    {
        Error = "invalid return value of require()";
        bSuccess = false;
    }
    else if (RetValues[0].GetType() != LUA_TTABLE)
    {
        Error = FString("table needed but got ");
        if(RetValues[0].GetType() == LUA_TSTRING)
            Error += UTF8_TO_TCHAR(RetValues[0].Value<const char*>());
        else
            Error += UTF8_TO_TCHAR(lua_typename(L, RetValues[0].GetType()));
        bSuccess = false;
    }
    else
    {
        bSuccess = BindInternal(Class, InModuleName, bMultipleLuaBind, Error);                             // bind!!!
    }

    if (bSuccess)
    {   
        FString RealModuleName = *ModuleNames.Find(Class);

        // create a Lua instance for this UObject
        Env->GetObjectRegistry()->Bind(Object, TCHAR_TO_UTF8(*RealModuleName));

        // try call user first user function handler
        int32 FunctionRef = PushFunction(L, Object, "Initialize");                  // push hard coded Lua function 'Initialize'
        if (FunctionRef != LUA_NOREF)
        {
            if (InitializerTableRef != LUA_NOREF)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, InitializerTableRef);             // push a initializer table if necessary
            }
            else
            {
                lua_pushnil(L);
            }
            bool bResult = ::CallFunction(L, 2, 0);                                 // call 'Initialize'
            if (!bResult)
            {
                UE_LOG(LogUnLua, Warning, TEXT("Failed to call 'Initialize' function!"));
            }
            luaL_unref(L, LUA_REGISTRYINDEX, FunctionRef);
        }
    }
    else
    {
        UE_LOG(LogUnLua, Warning, TEXT("Failed to attach %s module for object %s,%p!\n%s"), InModuleName, *Object->GetName(), Object, *Error);
    }

    return bSuccess;
}

void UUnLuaManager::OnWorldCleanup(UWorld* World, bool bArg, bool bCond)
{
    if (Env)
        Env->GC();
}

/**
 * Remove binded UObjects
 */
void UUnLuaManager::NotifyUObjectDeleted(const UObjectBase *Object)
{
    UClass* Class = (UClass*)Object;

    FString ModuleName;
    if (ModuleNames.RemoveAndCopyValue(Class, ModuleName))
    {
        Classes.Remove(ModuleName);
        ClearLoadedModule(Env->GetMainState(), TCHAR_TO_UTF8(*ModuleName));
    }
}

/**
 * Clean up...
 */
void UUnLuaManager::Cleanup()
{
    Env = nullptr;
    ModuleNames.Empty();
    Classes.Empty();
    ModuleFunctions.Empty();
}

/**
 * Clean up everything linked to the target UClass
 */
void UUnLuaManager::CleanUpByClass(UClass *Class)
{
    if (!Class)
        return;

    const FString *ModuleNamePtr = ModuleNames.Find(Class);
    if (!ModuleNamePtr)
        return;

    FString ModuleName = *ModuleNamePtr;
    Classes.Remove(ModuleName);
    ModuleFunctions.Remove(ModuleName);
    ModuleNames.Remove(Class);
}

/**
 * Get all default Axis/Action inputs
 */
void UUnLuaManager::GetDefaultInputs()
{
    UInputSettings *DefaultIS = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
    TArray<FName> AxisNames, ActionNames;
    DefaultIS->GetAxisNames(AxisNames);
    DefaultIS->GetActionNames(ActionNames);
    for (auto AxisName : AxisNames)
    {
        DefaultAxisNames.Add(AxisName);
    }
    for (auto ActionName : ActionNames)
    {
        DefaultActionNames.Add(ActionName);
    }
}

/**
 * Clean up all default Axis/Action inputs
 */
void UUnLuaManager::CleanupDefaultInputs()
{
    DefaultAxisNames.Empty();
    DefaultActionNames.Empty();
}

/**
 * Replace inputs
 */
bool UUnLuaManager::ReplaceInputs(AActor *Actor, UInputComponent *InputComponent)
{
    if (!Actor || !InputComponent)
    {
        return false;
    }

    UClass *Class = Actor->GetClass();
    FString* ModuleNamePtr = ModuleNames.Find(Class);
    if (!ModuleNamePtr)
        return false;
    TSet<FName> *LuaFunctionsPtr = ModuleFunctions.Find(*ModuleNamePtr);
    check(LuaFunctionsPtr);

    ReplaceActionInputs(Actor, InputComponent, *LuaFunctionsPtr);       // replace action inputs
    ReplaceKeyInputs(Actor, InputComponent, *LuaFunctionsPtr);          // replace key inputs
    ReplaceAxisInputs(Actor, InputComponent, *LuaFunctionsPtr);         // replace axis inputs
    ReplaceTouchInputs(Actor, InputComponent, *LuaFunctionsPtr);        // replace touch inputs
    ReplaceAxisKeyInputs(Actor, InputComponent, *LuaFunctionsPtr);      // replace AxisKey inputs
    ReplaceVectorAxisInputs(Actor, InputComponent, *LuaFunctionsPtr);   // replace VectorAxis inputs
    ReplaceGestureInputs(Actor, InputComponent, *LuaFunctionsPtr);      // replace gesture inputs

    return true;
}

/**
 * Callback when a map is loaded
 */
void UUnLuaManager::OnMapLoaded(UWorld *World)
{
    ENetMode NetMode = World->GetNetMode();
    if (NetMode == NM_DedicatedServer)
    {
        return;
    }

    const TArray<ULevel*> &Levels = World->GetLevels();
    for (ULevel *Level : Levels)
    {
        // replace input defined in ALevelScriptActor::InputComponent if necessary
        ALevelScriptActor *LSA = Level->GetLevelScriptActor();
        if (LSA && LSA->InputEnabled() && LSA->InputComponent)
        {
            ReplaceInputs(LSA, LSA->InputComponent);
        }
    }
}

/**
 * Callback for completing a latent function
 */
void UUnLuaManager::OnLatentActionCompleted(int32 LinkID)
{
    Env->ResumeThread(LinkID); // resume a coroutine
}

/**
 * Get target UCLASS for Lua binding
 */
UClass* UUnLuaManager::GetTargetClass(UClass *Class, UFunction **GetModuleNameFunc)
{
    static UClass *InterfaceClass = UUnLuaInterface::StaticClass();
    if (!Class || !Class->ImplementsInterface(InterfaceClass))
    {
        return nullptr;
    }
    UFunction *Func = Class->FindFunctionByName(FName("GetModuleName"));
    if (Func && Func->GetNativeFunc())
    {
        if (GetModuleNameFunc)
        {
            *GetModuleNameFunc = Func;
        }

        UClass *OuterClass = Func->GetOuterUClass();
        return OuterClass == InterfaceClass ? Class : OuterClass;
    }
    return nullptr;
}

/**
 * Bind a Lua module for a UObject
 */
bool UUnLuaManager::BindInternal(UClass* Class, const FString& InModuleName, bool bMultipleLuaBind, FString& Error)
{
    if (!Class)
        return false;

    // module may be already loaded for other class,etc muti bp bind to same lua
    FString RealModuleName = InModuleName;
    if (bMultipleLuaBind)
    {
        lua_State* L = Env->GetMainState();
        const int32 Type = GetLoadedModule(L, TCHAR_TO_UTF8(*InModuleName));
        if (Type != LUA_TTABLE) 
        {
            Error = FString::Printf(TEXT("table needed got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
            return false;
        }

        // generate new module for this module
        int16* NameIdx = RealModuleNames.Find(InModuleName);
        if (!NameIdx)
        {
            RealModuleNames.Add(InModuleName, 1);
            RealModuleName = FString::Printf(TEXT("%s_#%d"), *InModuleName, 1);
        }
        else
        {
            *NameIdx = *NameIdx + 1;
            RealModuleName = FString::Printf(TEXT("%s_#%d"), *InModuleName, *NameIdx);
        }

        // make a copy of lua module
        lua_newtable(L);
        lua_pushnil(L);
        while (lua_next(L, -3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -4);
        }

        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_pushvalue(L, -3);
        lua_setfield(L, -2, TCHAR_TO_UTF8(*RealModuleName));
        lua_pop(L, 3);
    }


    ModuleNames.Add(Class, RealModuleName);
    Classes.Add(RealModuleName, Class);

    TSet<FName> &LuaFunctions = ModuleFunctions.Add(RealModuleName);
    GetFunctionList(Env->GetMainState(), TCHAR_TO_UTF8(*RealModuleName), LuaFunctions);                         // get all functions defined in the Lua module
    TMap<FName, UFunction*> UEFunctions;
    ULuaFunction::GetOverridableFunctions(Class, UEFunctions);                                // get all overridable UFunctions

    OverrideFunctions(LuaFunctions, UEFunctions, Class);           // try to override UFunctions

    return ConditionalUpdateClass(Class, LuaFunctions, UEFunctions);
}


/**
 * Override special UFunctions
 */
bool UUnLuaManager::ConditionalUpdateClass(UClass *Class, const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions)
{
    check(Class);

    if (LuaFunctions.Num() < 1 || UEFunctions.Num() < 1)
    {
        return true;
    }

    if (Class->IsChildOf<UAnimInstance>())
    {
        for (const FName &FunctionName : LuaFunctions)
        {
            if (!UEFunctions.Find(FunctionName) && FunctionName.ToString().StartsWith(TEXT("AnimNotify_")))
            {
                ULuaFunction::Override(AnimNotifyFunc, Class, FunctionName); // override AnimNotify
            }
        }
    }

    return true;
}

/**
 * Override candidate UFunctions
 */
void UUnLuaManager::OverrideFunctions(const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions, UClass *OuterClass)
{
    for (const FName &LuaFuncName : LuaFunctions)
    {
        UFunction **Func = UEFunctions.Find(LuaFuncName);
        if (Func)
        {
            UFunction *Function = *Func;
            ULuaFunction::Override(Function, OuterClass, LuaFuncName);
        }
    }
}

/**
 * Replace action inputs
 */
void UUnLuaManager::ReplaceActionInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TSet<FName> ActionNames;
    int32 NumActionBindings = InputComponent->GetNumActionBindings();
    for (int32 i = 0; i < NumActionBindings; ++i)
    {
        FInputActionBinding &IAB = InputComponent->GetActionBinding(i);
        FName Name = GET_INPUT_ACTION_NAME(IAB);
        FString ActionName = Name.ToString();
        ActionNames.Add(Name);

        FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, SReadableInputEvent[IAB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputActionFunc, Class, FuncName);
            IAB.ActionDelegate.BindDelegate(Actor, FuncName);
        }

        if (!IS_INPUT_ACTION_PAIRED(IAB))
        {
            EInputEvent IE = IAB.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed;
            FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, SReadableInputEvent[IE]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputActionBinding AB(Name, IE);
                AB.ActionDelegate.BindDelegate(Actor, FuncName);
                InputComponent->AddActionBinding(AB);
            }
        }
    }

    EInputEvent IEs[] = { IE_Pressed, IE_Released };
    TSet<FName> DiffActionNames = DefaultActionNames.Difference(ActionNames);
    for (TSet<FName>::TConstIterator It(DiffActionNames); It; ++It)
    {
        FName ActionName = *It;
        for (int32 i = 0; i < 2; ++i)
        {
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName.ToString(), SReadableInputEvent[IEs[i]]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputActionBinding AB(ActionName, IEs[i]);
                AB.ActionDelegate.BindDelegate(Actor, FuncName);
                InputComponent->AddActionBinding(AB);
            }
        }
    }
}

/**
 * Replace key inputs
 */
void UUnLuaManager::ReplaceKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TArray<FKey> Keys;
    TArray<bool> PairedKeys;
    TArray<EInputEvent> InputEvents;
    for (FInputKeyBinding &IKB : InputComponent->KeyBindings)
    {
        int32 Index = Keys.Find(IKB.Chord.Key);
        if (Index == INDEX_NONE)
        {
            Keys.Add(IKB.Chord.Key);
            PairedKeys.Add(false);
            InputEvents.Add(IKB.KeyEvent);
        }
        else
        {
            PairedKeys[Index] = true;
        }

        FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *IKB.Chord.Key.ToString(), SReadableInputEvent[IKB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputActionFunc, Class, FuncName);
            IKB.KeyDelegate.BindDelegate(Actor, FuncName);
        }
    }

    for (int32 i = 0; i< Keys.Num(); ++i)
    {
        if (!PairedKeys[i])
        {
            EInputEvent IE = InputEvents[i] == IE_Pressed ? IE_Released : IE_Pressed;
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *Keys[i].ToString(), SReadableInputEvent[IE]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputKeyBinding IKB(FInputChord(Keys[i]), IE);
                IKB.KeyDelegate.BindDelegate(Actor, FuncName);
                InputComponent->KeyBindings.Add(IKB);
            }
        }
    }

    EInputEvent IEs[] = { IE_Pressed, IE_Released };
    for (const FKey &Key : AllKeys)
    {
        if (Keys.Find(Key) != INDEX_NONE)
        {
            continue;
        }
        for (int32 i = 0; i < 2; ++i)
        {
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *Key.ToString(), SReadableInputEvent[IEs[i]]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputKeyBinding IKB(FInputChord(Key), IEs[i]);
                IKB.KeyDelegate.BindDelegate(Actor, FuncName);
                InputComponent->KeyBindings.Add(IKB);
            }
        }
    }
}

/**
 * Replace axis inputs
 */
void UUnLuaManager::ReplaceAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TSet<FName> AxisNames;
    for (FInputAxisBinding &IAB : InputComponent->AxisBindings)
    {
        AxisNames.Add(IAB.AxisName);
        if (LuaFunctions.Find(IAB.AxisName))
        {
            ULuaFunction::Override(InputAxisFunc, Class, IAB.AxisName);
            IAB.AxisDelegate.BindDelegate(Actor, IAB.AxisName);
        }
    }

    TSet<FName> DiffAxisNames = DefaultAxisNames.Difference(AxisNames);
    for (TSet<FName>::TConstIterator It(DiffAxisNames); It; ++It)
    {
        if (LuaFunctions.Find(*It))
        {
            ULuaFunction::Override(InputAxisFunc, Class, *It);
            FInputAxisBinding &IAB = InputComponent->BindAxis(*It);
            IAB.AxisDelegate.BindDelegate(Actor, *It);
        }
    }
}

/**
 * Replace touch inputs
 */
void UUnLuaManager::ReplaceTouchInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TArray<EInputEvent> InputEvents = { IE_Pressed, IE_Released, IE_Repeat };        // IE_DoubleClick?
    for (FInputTouchBinding &ITB : InputComponent->TouchBindings)
    {
        InputEvents.Remove(ITB.KeyEvent);
        FName FuncName = FName(*FString::Printf(TEXT("Touch_%s"), SReadableInputEvent[ITB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputTouchFunc, Class, FuncName);
            ITB.TouchDelegate.BindDelegate(Actor, FuncName);
        }
    }

    for (EInputEvent IE : InputEvents)
    {
        FName FuncName = FName(*FString::Printf(TEXT("Touch_%s"), SReadableInputEvent[IE]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputTouchFunc, Class, FuncName);
            FInputTouchBinding ITB(IE);
            ITB.TouchDelegate.BindDelegate(Actor, FuncName);
            InputComponent->TouchBindings.Add(ITB);
        }
    }
}

/**
 * Replace axis key inputs
 */
void UUnLuaManager::ReplaceAxisKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputAxisKeyBinding &IAKB : InputComponent->AxisKeyBindings)
    {
        FName FuncName = IAKB.AxisKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputAxisFunc, Class, FuncName);
            IAKB.AxisDelegate.BindDelegate(Actor, FuncName);
        }
    }
}

/**
 * Replace vector axis inputs
 */
void UUnLuaManager::ReplaceVectorAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputVectorAxisBinding &IVAB : InputComponent->VectorAxisBindings)
    {
        FName FuncName = IVAB.AxisKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputVectorAxisFunc, Class, FuncName);
            IVAB.AxisDelegate.BindDelegate(Actor, FuncName);
        }
    }
}

/**
 * Replace gesture inputs
 */
void UUnLuaManager::ReplaceGestureInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputGestureBinding &IGB : InputComponent->GestureBindings)
    {
        FName FuncName = IGB.GestureKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputGestureFunc, Class, FuncName);
            IGB.GestureDelegate.BindDelegate(Actor, FuncName);
        }
    }
}
