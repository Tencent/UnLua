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

#include "DelegateHelper.h"
#include "LuaFunctionInjection.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "ReflectionUtils/PropertyDesc.h"
#include "lua.hpp"

void FSignatureDesc::MarkForDelete(bool bIgnoreBindings)
{
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FSignatureDesc::MarkForDelete %s"), *SignatureFunctionDesc->GetFunction()->GetName());
#endif

    if (!bIgnoreBindings && NumBindings > 1)
    {
        --NumBindings;              // dec bindings if there is more than one bindings.
        return;
    }
    else if (NumCalls > 0)
    {
        bPendingKill = true;        // raise pending kill flag if it's being called
        return;
    }

    FDelegateHelper::CleanUpByFunction(SignatureFunctionDesc->GetFunction());
}

void FSignatureDesc::Execute(UObject* Context, FFrame& Stack, void* RetValueAddress)
{
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FSignatureDesc::Execute %s"), *SignatureFunctionDesc->GetFunction()->GetName());
#endif

    if (SignatureFunctionDesc)
    {
        ++NumCalls;         // inc calls, so it won't be deleted during call
        SignatureFunctionDesc->CallLua(Context, Stack, RetValueAddress, false, false);
        --NumCalls;         // dec calls
        if (!NumCalls && bPendingKill)
        {
            if (NumBindings > 1) // NumBindings may increase when running CallLua
            {
                bPendingKill = false;
            }
            else
            {
                FDelegateHelper::CleanUpByFunction(SignatureFunctionDesc->GetFunction());       // clean up the delegate only if there is only one bindings.
            }
        }
    }
}

TMap<FScriptDelegate*, FDelegateProperty*> FDelegateHelper::Delegate2Property;
TMap<FMulticastDelegateType*, FMulticastDelegateProperty*> FDelegateHelper::MulticastDelegate2Property;
TMap<FScriptDelegate*, FFunctionDesc*> FDelegateHelper::Delegate2Signatures;
TMap<FMulticastDelegateType*, FFunctionDesc*> FDelegateHelper::MulticastDelegate2Signatures;
TMap<UFunction*, FSignatureDesc*> FDelegateHelper::Signatures;
TMap<FCallbackDesc, UFunction*> FDelegateHelper::Callbacks;
TMap<UFunction*, FCallbackDesc> FDelegateHelper::Function2Callback;
TMap<UClass*, TArray<UFunction*>> FDelegateHelper::Class2Functions;
TMap<UObject*, TArray<void*>> FDelegateHelper::Object2Delegates;
TMap<FMulticastDelegateType*, TArray<FCallbackDesc>> FDelegateHelper::MutiDelegates2Callback;

TMap<UObject*, TArray<void*>> FDelegateHelper::OwnerObject2Delegates;
TMap<UObject*, TArray<void*>> FDelegateHelper::OwnerObject2MulticastDelegates;
TMap<void*, TArray<UObject*>> FDelegateHelper::MulticastDelegates2BindObjects;

DEFINE_FUNCTION(FDelegateHelper::ProcessDelegate)
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    FSignatureDesc* SignatureDesc = nullptr;
    FMemory::Memcpy(&SignatureDesc, Stack.Code, sizeof(SignatureDesc));
    //Stack.SkipCode(sizeof(SignatureDesc));        // skip 'FSignatureDesc' pointer
#else
    FSignatureDesc** SignatureDescPtr = Signatures.Find(Stack.CurrentNativeFunction);   // find the signature
    FSignatureDesc* SignatureDesc = SignatureDescPtr ? *SignatureDescPtr : nullptr;
#endif
    if (SignatureDesc)
    {
#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::ProcessDelegate %s"), *Stack.CurrentNativeFunction->GetName());
#endif
        SignatureDesc->Execute(Context, Stack, (void*)RESULT_PARAM);     // fire the delegate
        return;
    }
    UE_LOG(LogUnLua, Warning, TEXT("Failed to process delegate (%s)!"), *Stack.CurrentNativeFunction->GetName());
}

FName FDelegateHelper::GetBindedFunctionName(const FCallbackDesc& Callback)
{
    UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        FSignatureDesc** SignatureDesc = Signatures.Find(*CallbackFuncPtr);
        ++((*SignatureDesc)->NumBindings);          // inc bindings
        return (*CallbackFuncPtr)->GetFName();      // return function name
    }
    return NAME_None;
}

void FDelegateHelper::PreBind(FScriptDelegate* ScriptDelegate, FDelegateProperty* Property)
{
    check(ScriptDelegate && Property);
    FDelegateProperty** PropertyPtr = Delegate2Property.Find(ScriptDelegate);
    if ((!PropertyPtr)
        || (*PropertyPtr != Property))
    {
        Delegate2Property.Add(ScriptDelegate, Property);
    }
}

bool FDelegateHelper::Bind(FScriptDelegate* ScriptDelegate, UObject* Object, const FCallbackDesc& Callback, int32 CallbackRef)
{
    FDelegateProperty** PropertyPtr = Delegate2Property.Find(ScriptDelegate);
    return PropertyPtr ? Bind(ScriptDelegate, *PropertyPtr, Object, Callback, CallbackRef) : false;
}

bool FDelegateHelper::Bind(FScriptDelegate* ScriptDelegate, FDelegateProperty* Property, UObject* Object, const FCallbackDesc& Callback, int32 CallbackRef)
{
    if (!ScriptDelegate || ScriptDelegate->IsBound() || !Property || !Object || !Callback.Class || CallbackRef == INDEX_NONE)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Delegate Bind! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

    UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
    if (!CallbackFuncPtr)
    {
#if UNLUA_ENABLE_DEBUG != 0
        lua_State* L = UnLua::GetState();
        lua_Debug ar;

        lua_getstack(L, 1, &ar);
        lua_getinfo(L, "nSl", &ar);
        int line = ar.linedefined;
        auto name = ar.source;

        FName FuncName(*FString::Printf(TEXT("LuaFunc:[%s:%d]_CppDelegate:[%s.%s_%s]"), ANSI_TO_TCHAR(name), line, *Object->GetName(), *Property->SignatureFunction->GetName(), *FGuid::NewGuid().ToString()));

        UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Bind %s %s with FuncName %s, Callback hash %d"), *Object->GetName(), *Property->SignatureFunction->GetName(), *FuncName.ToString(), Callback.Hash);
#else
        FName FuncName(*FString::Printf(TEXT("%s_%s"), *Property->GetName(), *FGuid::NewGuid().ToString()));
#endif

        ScriptDelegate->BindUFunction(Object, FuncName);                                    // bind a callback to the delegate
        CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback

        TArray<void*>& Delegates = Object2Delegates.FindOrAdd(Object);
        Delegates.Add(ScriptDelegate);
    }
#if UNLUA_ENABLE_DEBUG != 0
    else
    {
        UE_LOG(LogUnLua, Warning, TEXT("FDelegateHelper::Bind Callback of %d alread exist"), Callback.Hash);
    }
#endif
    return true;
}

void FDelegateHelper::Unbind(const FCallbackDesc& Callback)
{
    UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Unbind Callback of %d"), Callback.Hash);
#endif
        FSignatureDesc** SignatureDesc = Signatures.Find(*CallbackFuncPtr);
        if (SignatureDesc && *SignatureDesc)
        {
            (*SignatureDesc)->MarkForDelete(true);
        }
    }
}

void FDelegateHelper::Unbind(FScriptDelegate* ScriptDelegate)
{
    check(ScriptDelegate);
    if (!ScriptDelegate->IsBound())
    {
        return;
    }

    UObject* Object = ScriptDelegate->GetUObject();
    if (Object)
    {
        UFunction* Function = Object->FindFunction(ScriptDelegate->GetFunctionName());
        if (Function)
        {
            // try to delete the signature
            FSignatureDesc** SignatureDesc = Signatures.Find(Function);
            if (SignatureDesc && *SignatureDesc)
            {
                (*SignatureDesc)->MarkForDelete();
            }
        }

        if (Object2Delegates.Contains(Object))
        {
            Object2Delegates[Object].Remove(ScriptDelegate);
        }
    }

    Delegate2Property.Remove(ScriptDelegate);
    ScriptDelegate->Unbind();       // unbind the delegate
}

void FDelegateHelper::Unbind(FMulticastScriptDelegate* ScriptDelegate)
{
    check(ScriptDelegate);

    TArray<UObject*>* ObjectListPtr = MulticastDelegates2BindObjects.Find((void*)ScriptDelegate);
    if (ObjectListPtr)
    {
        for (UObject* Object : *ObjectListPtr)
        {
            if (Object2Delegates.Contains(Object))
            {
                Object2Delegates[Object].Remove(ScriptDelegate);
            }
        }
    }

    MulticastDelegates2BindObjects.Remove((void*)ScriptDelegate);
}


int32 FDelegateHelper::Execute(lua_State* L, FScriptDelegate* ScriptDelegate, int32 NumParams, int32 FirstParamIndex)
{
    check(ScriptDelegate);
    if (!ScriptDelegate->IsBound())
    {
        return 0;
    }

    FFunctionDesc* SignatureFunctionDesc = nullptr;
    FFunctionDesc** SignatureFunctionDescPtr = Delegate2Signatures.Find(ScriptDelegate);
    if (SignatureFunctionDescPtr)
    {
        SignatureFunctionDesc = *SignatureFunctionDescPtr;
    }
    else
    {
        FDelegateProperty** PropertyPtr = Delegate2Property.Find(ScriptDelegate);
        if (PropertyPtr)
        {
            UFunction* SignatureFunction = (*PropertyPtr)->SignatureFunction;
            SignatureFunctionDesc = GReflectionRegistry.RegisterFunction(SignatureFunction);
            Delegate2Signatures.Add(ScriptDelegate, SignatureFunctionDesc);
        }
    }

    if (SignatureFunctionDesc)
    {
        return SignatureFunctionDesc->ExecuteDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);        // fire the delegate
    }

    UE_LOG(LogUnLua, Warning, TEXT("Failed to execute FScriptDelegate!!!"));
    return 0;
}

void FDelegateHelper::PreAdd(FMulticastDelegateType* ScriptDelegate, FMulticastDelegateProperty* Property)
{
    check(ScriptDelegate && Property);
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if ((!PropertyPtr)
        || ((*PropertyPtr) != Property))
    {
        MulticastDelegate2Property.Add(ScriptDelegate, Property);
    }
}

bool FDelegateHelper::Add(FMulticastDelegateType* ScriptDelegate, UObject* Object, const FCallbackDesc& Callback, int32 CallbackRef)
{
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    return PropertyPtr ? Add(ScriptDelegate, *PropertyPtr, Object, Callback, CallbackRef) : false;
}

bool FDelegateHelper::Add(FMulticastDelegateType* ScriptDelegate, FMulticastDelegateProperty* Property, UObject* Object, const FCallbackDesc& Callback, int32 CallbackRef)
{
    if (!ScriptDelegate || !Property || !Object || !Callback.Class || CallbackRef == INDEX_NONE)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Delegate Add! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Add: Delegate[%p], Object[%p],%s"), ScriptDelegate, Object, *Object->GetName());
#endif

    UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
    if (!CallbackFuncPtr)
    {
        FName FuncName(*FString::Printf(TEXT("%s_%s"), *Property->GetName(), *FGuid::NewGuid().ToString()));

        FScriptDelegate DynamicDelegate;
        DynamicDelegate.BindUFunction(Object, FuncName);
        CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback
        TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // add a callback to the delegate

        TArray<void*>& ObjectDelegates = Object2Delegates.FindOrAdd(Object);
        ObjectDelegates.Add(ScriptDelegate);

        TArray<FCallbackDesc>& DelegateCallbacks = MutiDelegates2Callback.FindOrAdd(ScriptDelegate);
        DelegateCallbacks.Add(Callback);

        TArray<UObject*>& BindObjects = MulticastDelegates2BindObjects.FindOrAdd(ScriptDelegate);
        BindObjects.Add(Object);
    }

    return true;
}

void FDelegateHelper::Remove(FMulticastDelegateType* ScriptDelegate, UObject* Object, const FCallbackDesc& Callback)
{
    check(ScriptDelegate && Object);

    UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        if (GLuaCxt->IsUObjectValid((UObjectBase*)Object))
        {
            FMulticastDelegateProperty** Property = MulticastDelegate2Property.Find(ScriptDelegate);
            if ((!Property)
                || (!*Property))
            {
                return;
            }

            FPropertyDesc** PropertyDesc = FPropertyDesc::Property2Desc.Find(*Property);
            if ((!PropertyDesc)
                || (!GReflectionRegistry.IsDescValid(*PropertyDesc, DESC_PROPERTY)))
            {
                return;
            }

#if UNLUA_ENABLE_DEBUG != 0
            UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Remove: %p,%p,%s, %d"), ScriptDelegate, Object, *Object->GetName(), Callback.Hash);
#endif


            FScriptDelegate DynamicDelegate;
            DynamicDelegate.BindUFunction(Object, (*CallbackFuncPtr)->GetFName());
            TMulticastDelegateTraits<FMulticastDelegateType>::RemoveDelegate(*Property, DynamicDelegate, ScriptDelegate);    // remove a callback from the delegate
        }

        // try to delete the signature
        FSignatureDesc** SignatureDesc = Signatures.Find(*CallbackFuncPtr);
        if (SignatureDesc && *SignatureDesc)
        {
            (*SignatureDesc)->MarkForDelete();
        }
    }
}

void FDelegateHelper::Remove(UObject* Object)
{
    // first check if class exist
    if (Class2Functions.Contains(Object->GetClass()))
    {
        TArray<void*> Delegates;
        Object2Delegates.RemoveAndCopyValue(Object, Delegates);
        if (0 < Delegates.Num())
        {
            for (int i = 0; i < Delegates.Num(); ++i)
            {
                TArray<FCallbackDesc> DelegateCallbacks;
                MutiDelegates2Callback.RemoveAndCopyValue((FMulticastDelegateType*)Delegates[i], DelegateCallbacks);
                if (0 < DelegateCallbacks.Num())
                {
                    for (FCallbackDesc Callback : DelegateCallbacks)
                    {
                        Remove((FMulticastDelegateType*)Delegates[i], Object, Callback);
                    }
                }
                else
                {
                    Unbind((FScriptDelegate*)Delegates[i]);
                }
            }
        }
    }
}

void FDelegateHelper::Clear(FMulticastDelegateType* InScriptDelegate)
{
    if (!InScriptDelegate /*|| !InScriptDelegate->IsBound()*/)
    {
        return;
    }

    FMulticastDelegateProperty* Property = nullptr;
#if ENGINE_MINOR_VERSION > 22
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(InScriptDelegate);
    if (!PropertyPtr || !(*PropertyPtr))
    {
        return;     // invalid FMulticastDelegateProperty
    }
    Property = *PropertyPtr;
#endif

    TMulticastDelegateTraits<FMulticastDelegateType>::ClearDelegate(Property, InScriptDelegate);        // clear all callbacks

    TArray<FCallbackDesc> DelegateCallbacks;
    bool bSuccess = MutiDelegates2Callback.RemoveAndCopyValue(InScriptDelegate, DelegateCallbacks);
    if (bSuccess)
    {
        for (FCallbackDesc Callback : DelegateCallbacks)
        {
            // try to delete the signature
            UFunction** CallbackFuncPtr = Callbacks.Find(Callback);
            if (CallbackFuncPtr && *CallbackFuncPtr)
            {
                FSignatureDesc** SignatureDesc = Signatures.Find(*CallbackFuncPtr);
                if (SignatureDesc && *SignatureDesc)
                {
                    (*SignatureDesc)->MarkForDelete();
                }
            }
        }
    }
}

void FDelegateHelper::Broadcast(lua_State* L, FMulticastDelegateType* InScriptDelegate, int32 NumParams, int32 FirstParamIndex)
{
    check(InScriptDelegate);
#if ENGINE_MINOR_VERSION < 23           // todo: how to check it for 4.23.x and above...
    if (!InScriptDelegate->IsBound())
    {
        return;
    }
#endif

    FMulticastDelegateProperty* Property = nullptr;
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(InScriptDelegate);
    if (PropertyPtr)
    {
        Property = *PropertyPtr;
    }

    FFunctionDesc* SignatureFunctionDesc = nullptr;
    FFunctionDesc** SignatureFunctionDescPtr = MulticastDelegate2Signatures.Find(InScriptDelegate);
    if (SignatureFunctionDescPtr)
    {
        SignatureFunctionDesc = *SignatureFunctionDescPtr;
    }
    else
    {
        UFunction* SignatureFunction = Property->SignatureFunction;
        SignatureFunctionDesc = GReflectionRegistry.RegisterFunction(SignatureFunction);
        MulticastDelegate2Signatures.Add(InScriptDelegate, SignatureFunctionDesc);
    }

    if (SignatureFunctionDesc && Property)
    {
        FMulticastScriptDelegate* ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, InScriptDelegate);  // get target delegate
        SignatureFunctionDesc->BroadcastMulticastDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);        // fire the delegate
        return;
    }

    UE_LOG(LogUnLua, Warning, TEXT("Failed to broadcast multicast delegate!!!"));
}

void FDelegateHelper::AddDelegate(FMulticastDelegateType* ScriptDelegate, UObject* Object, const FCallbackDesc& Callback, FScriptDelegate DynamicDelegate)
{
    FMulticastDelegateProperty* Property = nullptr;
#if ENGINE_MINOR_VERSION > 22
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if (!PropertyPtr || !(*PropertyPtr))
    {
        return;     // invalid FMulticastDelegateProperty
    }
    Property = *PropertyPtr;
#endif
    TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // adds a callback to delegate's invocation list

    TArray<void*>& ObjectDelegates = Object2Delegates.FindOrAdd(Object);
    ObjectDelegates.Add(ScriptDelegate);

    TArray<FCallbackDesc>& DelegateCallbacks = MutiDelegates2Callback.FindOrAdd(ScriptDelegate);
    DelegateCallbacks.Add(Callback);

    TArray<UObject*>& BindObjects = MulticastDelegates2BindObjects.FindOrAdd(ScriptDelegate);
    BindObjects.Add(Object);
}

void FDelegateHelper::CleanUpByFunction(UFunction* Function)
{
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::CleanUpByFunction: [%p], [%s]"), Function, *Function->GetName());
#endif

    // cleanup all associated stuff of a UFunction
    FSignatureDesc* SignatureDesc = nullptr;
    if (Signatures.RemoveAndCopyValue(Function, SignatureDesc))
    {
        delete SignatureDesc;
    }

    FCallbackDesc Callback;
    if (Function2Callback.RemoveAndCopyValue(Function, Callback))
    {
        Callbacks.Remove(Callback);

        TArray<UFunction*>* FunctionsPtr = Class2Functions.Find(Callback.Class);
        if (FunctionsPtr)
        {
            FunctionsPtr->Remove(Function);
            if (FunctionsPtr->Num() < 1)
            {
                Class2Functions.Remove(Callback.Class);
            }
        }

        RemoveUFunction(Function, Callback.Class);       // remove the duplicated function
    }
}

void FDelegateHelper::CleanUpByClass(UClass* Class)
{
#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::CleanUpByClass: [%p], [%s]"), Class, *Class->GetName());
#endif
    // cleanup all associated stuff of a UClass
    TArray<UFunction*> Functions;
    if (Class2Functions.RemoveAndCopyValue(Class, Functions))
    {
        for (UFunction* Function : Functions)
        {
            CleanUpByFunction(Function);
        }
    }
}

void FDelegateHelper::Cleanup(bool bFullCleanup)
{
    // cleanup all stuff during level transition
    for (TMap<UClass*, TArray<UFunction*>>::TIterator It(Class2Functions); It; ++It)
    {
        CleanUpByClass(It.Key());
    }
    Class2Functions.Empty();
    Signatures.Empty();
    Callbacks.Empty();
    Function2Callback.Empty();

    for (TMap<FScriptDelegate*, FFunctionDesc*>::TIterator It(Delegate2Signatures); It; ++It)
    {
        GReflectionRegistry.UnRegisterFunction(It.Value()->GetFunction());
    }
    Delegate2Signatures.Empty();

    for (TMap<FMulticastDelegateType*, FFunctionDesc*>::TIterator It(MulticastDelegate2Signatures); It; ++It)
    {
        GReflectionRegistry.UnRegisterFunction(It.Value()->GetFunction());
    }
    MulticastDelegate2Signatures.Empty();


    Delegate2Property.Empty();
    MulticastDelegate2Property.Empty();
    Object2Delegates.Empty();
    MutiDelegates2Callback.Empty();
    MulticastDelegates2BindObjects.Empty();
    OwnerObject2MulticastDelegates.Empty();
    OwnerObject2Delegates.Empty();
}

void FDelegateHelper::NotifyUObjectDeleted(const UObject* InObject)
{
    if (OwnerObject2Delegates.Contains(InObject))
    {
        for (void* delegate : OwnerObject2Delegates[InObject])
        {
            Unbind((FScriptDelegate*)delegate);
        }
        OwnerObject2Delegates.Remove(InObject);
    }

    if (OwnerObject2MulticastDelegates.Contains(InObject))
    {
        for (void* delegate : OwnerObject2MulticastDelegates[InObject])
        {
            Unbind((FMulticastScriptDelegate*)delegate);
        }
        OwnerObject2MulticastDelegates.Remove(InObject);
    }

    Object2Delegates.Remove(InObject);
}

void FDelegateHelper::AddDelegateOwnerObject(void* Delegate, UObject* Object)
{
    TArray<void*>& Delegates = OwnerObject2Delegates.FindOrAdd(Object);
    Delegates.Add(Delegate);
}

void FDelegateHelper::AddMulticastDelegateOwnerObject(void* Delegate, UObject* Object)
{
    TArray<void*>& MulticastDelegates = OwnerObject2MulticastDelegates.FindOrAdd(Object);
    MulticastDelegates.Add(Delegate);
}

/**
 * 1. Create a new signature UFunction
 * 2. Set a custom thunk function for the new signature
 * 3. Create a signature descriptor
 * 4. Update function flags for the new signature if necessary
 * 5. Update cached infos
 */
void FDelegateHelper::CreateSignature(UFunction* TemplateFunction, FName FuncName, const FCallbackDesc& Callback, int32 CallbackRef)
{
    UFunction* SignatureFunction = DuplicateUFunction(TemplateFunction, Callback.Class, FuncName);      // duplicate the signature UFunction
    SignatureFunction->Script.Empty();

    FSignatureDesc* SignatureDesc = new FSignatureDesc;
    SignatureDesc->SignatureFunctionDesc = GReflectionRegistry.RegisterFunction(SignatureFunction, CallbackRef);
    SignatureDesc->CallbackRef = CallbackRef;
    Signatures.Add(SignatureFunction, SignatureDesc);

    OverrideUFunction(SignatureFunction, (FNativeFuncPtr)&FDelegateHelper::ProcessDelegate, SignatureDesc, false);      // set custom thunk function for the duplicated UFunction

    uint8 NumRefProperties = SignatureDesc->SignatureFunctionDesc->GetNumRefProperties();
    if (NumRefProperties > 0)
    {
        SignatureFunction->FunctionFlags |= FUNC_HasOutParms;        // 'FUNC_HasOutParms' will not be set for signature function even if it has out parameters
    }

    Callbacks.Add(Callback, SignatureFunction);
    Function2Callback.Add(SignatureFunction, Callback);

    TArray<UFunction*>& Functions = Class2Functions.FindOrAdd(Callback.Class);
    Functions.Add(SignatureFunction);
}