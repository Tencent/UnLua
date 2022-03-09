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

#include "UnLuaManager.h"
#include "UnLua.h"
#include "UnLuaInterface.h"
#include "LuaCore.h"
#include "LuaContext.h"
#include "LuaFunctionInjection.h"
#include "DelegateHelper.h"
#include "UEObjectReferencer.h"
#include "GameFramework/InputSettings.h"
#include "Components/InputComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/LevelScriptActor.h"

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

    PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddLambda([this]()
    {
        this->PostGarbageCollect();
    });
}

UUnLuaManager::~UUnLuaManager()
{
    FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);
}


/**
 * Bind a Lua module for a UObject
 */
bool UUnLuaManager::Bind(UObjectBaseUtility *Object, UClass *Class, const TCHAR *InModuleName, int32 InitializerTableRef)
{   
    if (!Object || !Class)
    {
        UE_LOG(LogUnLua, Warning, TEXT("Invalid target object!"));
        return false;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("UUnLuaManager::Bind : %p,%s,%s"), Object, *Object->GetName(),InModuleName);
#endif
    
    bool bSuccess = true;
    lua_State *L = *GLuaCxt;

    bool bMultipleLuaBind = false;
    UClass** BindedClass = Classes.Find(InModuleName);
    if ((BindedClass)
        &&(*BindedClass != Object->GetClass()))
    {
        bMultipleLuaBind = true;
    }

    if (!RegisterClass(L, Class))              // register class first
    {
        return false;
    }

    // try bind lua if not bind or use a copyed table
    UnLua::FLuaRetValues RetValues = UnLua::Call(L, "require", TCHAR_TO_UTF8(InModuleName));    // require Lua module
    FString Error;
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
        bSuccess = BindInternal(Object, Class, InModuleName, true, bMultipleLuaBind, Error);                             // bind!!!
    }

    if (bSuccess)
    {   
        bool bDerivedClassBinded = false;
        if (Object->GetClass() != Class)
        {
            bDerivedClassBinded = true;
            OnDerivedClassBinded(Object->GetClass(), Class);
        }

        FString RealModuleName = *ModuleNames.Find(Class);

        GLuaCxt->AddModuleName(*RealModuleName);                                       // record this required module

        // create a Lua instance for this UObject
        int32 ObjectRef = NewLuaObject(L, Object, bDerivedClassBinded ? Class : nullptr, TCHAR_TO_UTF8(*RealModuleName));

        AddAttachedObject(Object, ObjectRef);                                       // record this binded UObject

        // try call user first user function handler
        bool bResult = false;
        int32 FunctionRef = PushFunction(L, Object, "Initialize");                  // push hard coded Lua function 'Initialize'
        if (FunctionRef != INDEX_NONE)
        {
            if (InitializerTableRef != INDEX_NONE)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, InitializerTableRef);             // push a initializer table if necessary
            }
            else
            {
                lua_pushnil(L);
            }
            bResult = ::CallFunction(L, 2, 0);                                 // call 'Initialize'
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

/**
 * Callback for 'Hotfix'
 */
bool UUnLuaManager::OnModuleHotfixed(const TCHAR *InModuleName)
{
    TArray<FString> _ModuleNames;
    _ModuleNames.Add(InModuleName);
    int16* NameIdx = RealModuleNames.Find(InModuleName);
    for (int16 i = 1; i < *NameIdx; ++i)
    {
        _ModuleNames.Add(FString::Printf(TEXT("%s_#%d"),InModuleName,i));
    }

    for (int i = 0; i < _ModuleNames.Num(); ++i)
    {
        UClass** ClassPtr = Classes.Find(_ModuleNames[i]);
        if (ClassPtr)
        {
            lua_State* L = *GLuaCxt;
            TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ModuleName(InModuleName);

            TSet<FName> LuaFunctions;
            bool bSuccess = GetFunctionList(L, ModuleName.Get(), LuaFunctions);                 // get all functions in this Lua module/table
            if (!bSuccess)
            {
                continue;
            }

            TSet<FName>* LuaFunctionsPtr = ModuleFunctions.Find(InModuleName);
            check(LuaFunctionsPtr);
            TSet<FName> NewFunctions = LuaFunctions.Difference(*LuaFunctionsPtr);               // get new added Lua functions
            if (NewFunctions.Num() > 0)
            {
                UClass* Class = *ClassPtr;
                TMap<FName, UFunction*>* UEFunctionsPtr = OverridableFunctions.Find(Class);     // get all overridable UFunctions
                check(UEFunctionsPtr);
                for (const FName& LuaFuncName : NewFunctions)
                {
                    UFunction** Func = UEFunctionsPtr->Find(LuaFuncName);
                    if (Func)
                    {
                        OverrideFunction(*Func, Class, LuaFuncName);                            // override the UFunction
                    }
                }

                bSuccess = ConditionalUpdateClass(Class, NewFunctions, *UEFunctionsPtr);        // update class conditionally
            }
        }
    }
        
    return true;
}

/**
 * Remove binded UObjects
 */
void UUnLuaManager::NotifyUObjectDeleted(const UObjectBase *Object, bool bClass)
{
    if (bClass)
    {
        //OnClassCleanup((UClass*)Object);
    }
    else
    {
        DeleteLuaObject(*GLuaCxt, (UObjectBaseUtility*)Object);        // delete the Lua instance (table)
    }
}

/**
 * Clean up...
 */
void UUnLuaManager::Cleanup(UWorld *InWorld, bool bFullCleanup)
{
    AttachedObjects.Empty();
    AttachedActors.Empty();

    ModuleNames.Empty();
    Classes.Empty();
    OverridableFunctions.Empty();
    ModuleFunctions.Empty();

    CleanupDuplicatedFunctions();       // clean up duplicated UFunctions
    CleanupCachedNatives();             // restore cached thunk functions
    CleanupCachedScripts();             // restore cached scripts

#if !ENABLE_CALL_OVERRIDDEN_FUNCTION
    New2TemplateFunctions.Empty();
#endif
}

/**
 * Clean up everything linked to the target UClass
 */
void UUnLuaManager::CleanUpByClass(UClass *Class)
{
    if (!Class)
    {
        return;
    }

    const FString *ModuleNamePtr = ModuleNames.Find(Class);
    if (!ModuleNamePtr)
        return;

    FString ModuleName = *ModuleNamePtr;

    Classes.Remove(ModuleName);
    ModuleFunctions.Remove(ModuleName);

    TMap<FName, UFunction*> FunctionMap;
    OverridableFunctions.RemoveAndCopyValue(Class, FunctionMap);

    for (TMap<FName, UFunction*>::TIterator It(FunctionMap); It; ++It)
    {
        UFunction* Function = It.Value();
        if (Function->GetOuter() != Class)
            continue;
        FNativeFuncPtr NativeFuncPtr = nullptr;
        if (CachedNatives.RemoveAndCopyValue(Function, NativeFuncPtr))
        {
            ResetUFunction(Function, NativeFuncPtr);
        }
    }

    TArray<TWeakObjectPtr<UFunction>> Functions;
    if (DuplicatedFunctions.RemoveAndCopyValue(Class, Functions))
    {
        if (!Class->HasAnyFlags(RF_BeginDestroyed))
            RemoveDuplicatedFunctions(Class, Functions);
    }

    OnClassCleanup(Class);

    FDelegateHelper::CleanUpByClass(Class);

    ClearLoadedModule(*GLuaCxt, TCHAR_TO_UTF8(*ModuleName));

    ModuleNames.Remove(Class);
}

/**
 * Clean duplicated UFunctions
 */
void UUnLuaManager::CleanupDuplicatedFunctions()
{
    for (TMap<UClass*, TArray<TWeakObjectPtr<UFunction>>>::TIterator It(DuplicatedFunctions); It; ++It)
    {
        OnClassCleanup(It.Key());
        if (!It.Key()->HasAnyFlags(RF_BeginDestroyed))
            RemoveDuplicatedFunctions(It.Key(), It.Value());
    }
    DuplicatedFunctions.Empty();
    Base2DerivedClasses.Empty();
    Derived2BaseClasses.Empty();
}

/**
 * Restore cached thunk functions
 */
void UUnLuaManager::CleanupCachedNatives()
{
    TArray<TWeakObjectPtr<UFunction>> Functions;
    CachedNatives.GetKeys(Functions);
    for (auto It = CachedNatives.CreateIterator(); It; ++It)
    {
        if (It.Key().IsValid())
            ResetUFunction(It.Key().Get(), It.Value());
    }
    CachedNatives.Empty();
}

/**
 * Restore cached scripts
 */
void UUnLuaManager::CleanupCachedScripts()
{
    for (auto It = CachedScripts.CreateIterator(); It; ++It)
    {
        if (It.Key().IsValid())
        {
            It.Key()->Script = It.Value();
        }
    }
    CachedScripts.Empty();
}

/**
 * Cleanup intermediate data linked to a UClass
 */
void UUnLuaManager::OnClassCleanup(UClass *Class)
{
    UClass *BaseClass = nullptr;
    if (Derived2BaseClasses.RemoveAndCopyValue(Class, BaseClass))
    {
        TArray<UClass*> *DerivedClasses = Base2DerivedClasses.Find(BaseClass);
        if (DerivedClasses)
        {
            DerivedClasses->Remove(Class);
        }
    }

    TArray<UClass*> DerivedClasses;
    if (Base2DerivedClasses.RemoveAndCopyValue(Class, DerivedClasses))
    {
        for (UClass *DerivedClass : DerivedClasses)
        {
            DerivedClass->ClearFunctionMapsCaches();            // clean up cached UFunctions of super class
        }
    }
}

/**
 * Reset a UFunction
 */
void UUnLuaManager::ResetUFunction(UFunction *Function, FNativeFuncPtr NativeFuncPtr)
{
    if (!Function->HasAllFlags(RF_BeginDestroyed))
    {
        Function->SetNativeFunc(NativeFuncPtr);

        if (Function->Script.Num() > 0 && Function->Script[0] == EX_CallLua)
        {
            Function->Script.Empty();
        }

        TArray<uint8> Script;
        if (CachedScripts.RemoveAndCopyValue(Function, Script))
        {
            Function->Script = Script;
        }
    }

    GReflectionRegistry.UnRegisterFunction(Function);

#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    UFunction* OverriddenFunc = GReflectionRegistry.RemoveOverriddenFunction(Function);
    if (!OverriddenFunc)
        return;
    if (OverriddenFunc->IsValidLowLevel())
        RemoveUFunction(OverriddenFunc, OverriddenFunc->GetOuterUClass());
    else
        GReflectionRegistry.UnRegisterFunction(OverriddenFunc);
#endif
}

/**
 * Remove duplicated UFunctions
 */
void UUnLuaManager::RemoveDuplicatedFunctions(UClass *Class, TArray<TWeakObjectPtr<UFunction>> &Functions)
{
    for (auto Function : Functions)
    {
        if (!Function.IsValid())
            continue;
        RemoveUFunction(Function.Get(), Class); // clean up duplicated UFunction
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
        GReflectionRegistry.RemoveOverriddenFunction(Function.Get());
#endif
    }
}

void UUnLuaManager::PostGarbageCollect()
{
    for (auto It = CachedScripts.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }

    for (auto It = CachedNatives.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}

/**
 * Post process for cleaning up
 */
void UUnLuaManager::PostCleanup()
{

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
    if (!Actor || !InputComponent || !AttachedObjects.Find(Actor))
    {
        return false;
    }

    UClass *Class = Actor->GetClass();
    FString *ModuleNamePtr = ModuleNames.Find(Class);
    if (!ModuleNamePtr)
    {
        UClass **SuperClassPtr = Derived2BaseClasses.Find(Class);
        if (!SuperClassPtr || !(*SuperClassPtr))
        {
            return false;
        }
        ModuleNamePtr = ModuleNames.Find(*SuperClassPtr);
    }
    check(ModuleNamePtr);
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
    /*for (AActor *Actor : AttachedActors)
    {	
		if (GLuaCxt->IsUObjectValid(Actor))
		{
			if (!Actor->OnDestroyed.IsAlreadyBound(this, &UUnLuaManager::OnActorDestroyed))
			{
				Actor->OnDestroyed.AddDynamic(this, &UUnLuaManager::OnActorDestroyed);      // bind a callback for destroying actor
			}
		}
    }*/

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
 * Callback for spawning an actor
 */
void UUnLuaManager::OnActorSpawned(AActor *Actor)
{
    if (!GLuaCxt->IsEnable())
    {
        return;
    }

    Actor->OnDestroyed.AddDynamic(this, &UUnLuaManager::OnActorDestroyed);      // bind a callback for destroying actor
}

/**
 * Callback for destroying an actor
 */
void UUnLuaManager::OnActorDestroyed(AActor *Actor)
{
    if (!GLuaCxt->IsEnable())
    {
        return;
    }

    int32 Num = AttachedActors.Remove(Actor);
    if (Num > 0)
    {
        ReleaseAttachedObjectLuaRef(Actor);
        DeleteUObjectRefs(UnLua::GetState(),Actor);   // remove record of this actor
    }
}

/**
 * Callback for completing a latent function
 */
void UUnLuaManager::OnLatentActionCompleted(int32 LinkID)
{
    GLuaCxt->ResumeThread(LinkID);              // resume a coroutine
}

/**
 * Notify that a derived class is binded to its base class
 */
void UUnLuaManager::OnDerivedClassBinded(UClass *DerivedClass, UClass *BaseClass)
{
    TArray<UClass*> &DerivedClasses = Base2DerivedClasses.FindOrAdd(BaseClass);
    do
    {
        if (DerivedClasses.Find(DerivedClass) != INDEX_NONE)
        {
            break;
        }
        Derived2BaseClasses.Add(DerivedClass, BaseClass);
        DerivedClasses.Add(DerivedClass);
        DerivedClass = DerivedClass->GetSuperClass();
    } while (DerivedClass != BaseClass);
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
bool UUnLuaManager::BindInternal(UObjectBaseUtility* Object, UClass* Class, const FString& InModuleName, bool bNewCreated, bool bMultipleLuaBind, FString& Error)
{
    if (!Object || !Class)
    {
        return false;
    }

    // module may be already loaded for other class,etc muti bp bind to same lua
    FString RealModuleName = InModuleName;
    if (bMultipleLuaBind)
    {
        lua_State* L = UnLua::GetState();
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
    GetFunctionList(UnLua::GetState(), TCHAR_TO_UTF8(*RealModuleName), LuaFunctions);                         // get all functions defined in the Lua module
    TMap<FName, UFunction*> &UEFunctions = OverridableFunctions.Add(Class);
    GetOverridableFunctions(Class, UEFunctions);                                // get all overridable UFunctions

    OverrideFunctions(LuaFunctions, UEFunctions, Class, bNewCreated);           // try to override UFunctions

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
                AddFunction(AnimNotifyFunc, Class, FunctionName);           // override AnimNotify
            }
        }
    }

    return true;
}

/**
 * Override candidate UFunctions
 */
void UUnLuaManager::OverrideFunctions(const TSet<FName> &LuaFunctions, TMap<FName, UFunction*> &UEFunctions, UClass *OuterClass, bool bCheckFuncNetMode)
{
    for (const FName &LuaFuncName : LuaFunctions)
    {
        UFunction **Func = UEFunctions.Find(LuaFuncName);
        if (Func)
        {
            UFunction *Function = *Func;
            OverrideFunction(Function, OuterClass, LuaFuncName);
        }
    }
}

/**
 * Override a UFunction
 */
void UUnLuaManager::OverrideFunction(UFunction *TemplateFunction, UClass *OuterClass, FName NewFuncName)
{
    if (TemplateFunction->GetOuter() != OuterClass)
    {
//#if UE_BUILD_SHIPPING || UE_BUILD_TEST
        if (TemplateFunction->Script.Num() > 0 && TemplateFunction->Script[0] == EX_CallLua)
        {
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
            TemplateFunction = GReflectionRegistry.FindOverriddenFunction(TemplateFunction);
#else
            TemplateFunction = New2TemplateFunctions.FindChecked(TemplateFunction);
#endif
        }
//#endif
        AddFunction(TemplateFunction, OuterClass, NewFuncName);     // add a duplicated UFunction to child UClass
    }
    else
    {
        ReplaceFunction(TemplateFunction, OuterClass);              // replace thunk function and insert opcodes
    }
}

/**
 * Add a duplicated UFunction to UClass
 */
void UUnLuaManager::AddFunction(UFunction *TemplateFunction, UClass *OuterClass, FName NewFuncName)
{
    UFunction *Func = OuterClass->FindFunctionByName(NewFuncName, EIncludeSuperFlag::ExcludeSuper);
    if (!Func)
    {
        if (TemplateFunction->HasAnyFunctionFlags(FUNC_Native))
        {
            // call this before duplicate UFunction that has FUNC_Native to eliminate "Failed to bind native function" warnings.
            OuterClass->AddNativeFunction(*NewFuncName.ToString(), (FNativeFuncPtr)&FLuaInvoker::execCallLua);
        }

        UFunction *NewFunc = DuplicateUFunction(TemplateFunction, OuterClass, NewFuncName); // duplicate a UFunction
        if (!NewFunc->HasAnyFunctionFlags(FUNC_Native) && NewFunc->Script.Num() > 0)
        {
            NewFunc->Script.Empty(3);                               // insert opcodes for non-native UFunction only
        }
        OverrideUFunction(NewFunc, (FNativeFuncPtr)&FLuaInvoker::execCallLua, GReflectionRegistry.RegisterFunction(NewFunc));   // replace thunk function and insert opcodes
        TArray<TWeakObjectPtr<UFunction>> &DuplicatedFuncs = DuplicatedFunctions.FindOrAdd(OuterClass);
        DuplicatedFuncs.AddUnique(NewFunc);
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
        GReflectionRegistry.AddOverriddenFunction(NewFunc, TemplateFunction);
#else
        New2TemplateFunctions.Add(NewFunc, TemplateFunction);
#endif
    }
}

/**
 * Replace thunk function and insert opcodes
 */
void UUnLuaManager::ReplaceFunction(UFunction *TemplateFunction, UClass *OuterClass)
{
    if (TemplateFunction->GetNativeFunc() == FLuaInvoker::execCallLua)
        return;

    FNativeFuncPtr *NativePtr = CachedNatives.Find(TemplateFunction);
    if (!NativePtr)
    {
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
        FName NewFuncName(*FString::Printf(TEXT("%s%s"), *TemplateFunction->GetName(), TEXT("Copy")));
        if (TemplateFunction->HasAnyFunctionFlags(FUNC_Native))
        {
            // call this before duplicate UFunction that has FUNC_Native to eliminate "Failed to bind native function" warnings.
            OuterClass->AddNativeFunction(*NewFuncName.ToString(), TemplateFunction->GetNativeFunc());
        }
        UFunction *NewFunc = DuplicateUFunction(TemplateFunction, OuterClass, NewFuncName);
        GReflectionRegistry.AddOverriddenFunction(TemplateFunction, NewFunc);
#endif
        CachedNatives.Add(TemplateFunction, TemplateFunction->GetNativeFunc());
        if (!TemplateFunction->HasAnyFunctionFlags(FUNC_Native) && TemplateFunction->Script.Num() > 0)
        {
            CachedScripts.Add(TemplateFunction, TemplateFunction->Script);
            TemplateFunction->Script.Empty(3);
        }
        OverrideUFunction(TemplateFunction, (FNativeFuncPtr)&FLuaInvoker::execCallLua, GReflectionRegistry.RegisterFunction(TemplateFunction));
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
            AddFunction(InputActionFunc, Class, FuncName);
            IAB.ActionDelegate.BindDelegate(Actor, FuncName);
        }

        if (!IS_INPUT_ACTION_PAIRED(IAB))
        {
            EInputEvent IE = IAB.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed;
            FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, SReadableInputEvent[IE]));
            if (LuaFunctions.Find(FuncName))
            {
                AddFunction(InputActionFunc, Class, FuncName);
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
                AddFunction(InputActionFunc, Class, FuncName);
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
            AddFunction(InputActionFunc, Class, FuncName);
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
                AddFunction(InputActionFunc, Class, FuncName);
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
                AddFunction(InputActionFunc, Class, FuncName);
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
            AddFunction(InputAxisFunc, Class, IAB.AxisName);
            IAB.AxisDelegate.BindDelegate(Actor, IAB.AxisName);
        }
    }

    TSet<FName> DiffAxisNames = DefaultAxisNames.Difference(AxisNames);
    for (TSet<FName>::TConstIterator It(DiffAxisNames); It; ++It)
    {
        if (LuaFunctions.Find(*It))
        {
            AddFunction(InputAxisFunc, Class, *It);
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
            AddFunction(InputTouchFunc, Class, FuncName);
            ITB.TouchDelegate.BindDelegate(Actor, FuncName);
        }
    }

    for (EInputEvent IE : InputEvents)
    {
        FName FuncName = FName(*FString::Printf(TEXT("Touch_%s"), SReadableInputEvent[IE]));
        if (LuaFunctions.Find(FuncName))
        {
            AddFunction(InputTouchFunc, Class, FuncName);
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
            AddFunction(InputAxisFunc, Class, FuncName);
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
            AddFunction(InputVectorAxisFunc, Class, FuncName);
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
            AddFunction(InputGestureFunc, Class, FuncName);
            IGB.GestureDelegate.BindDelegate(Actor, FuncName);
        }
    }
}

/**
 * Record a binded UObject
 */
void UUnLuaManager::AddAttachedObject(UObjectBaseUtility *Object, int32 ObjectRef)
{
    check(Object);

    GObjectReferencer.AddObjectRef((UObject*)Object);

    AttachedObjects.Add(Object, ObjectRef);

    if (Object->IsA<AActor>())
    {
        AttachedActors.Add((AActor*)Object);
    }
}

/**
 * Get lua ref of a recorded binded UObject
 */
void UUnLuaManager::ReleaseAttachedObjectLuaRef(UObjectBaseUtility* Object)
{
    check(Object);
    
    GObjectReferencer.RemoveObjectRef((UObject*)Object);
    
    int32* ObjectLuaRef = AttachedObjects.Find(Object);
    if ((ObjectLuaRef)
        &&(*ObjectLuaRef != LUA_REFNIL))
    {   
#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogUnLua, Log, TEXT("ReleaseAttachedObjectLuaRef : %s,%p,%d"), *Object->GetName(), Object, *ObjectLuaRef);
#endif
        luaL_unref(UnLua::GetState(), LUA_REGISTRYINDEX, *ObjectLuaRef);
        AttachedObjects.Remove(Object);
    }
}
