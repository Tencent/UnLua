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

/**
 * archive used to get invocation list from a multicast delegate
 */
class FInvocationListReader : public FArchive
{
public:
    FInvocationListReader()
    {
        SetIsSaving(true);
    }

    /**
     * Collect objects
     */
    virtual FArchive& operator<<(struct FWeakObjectPtr &Value) override
    {
        Objects.Add(Value.Get());
        return *this;
    }

    /**
     * Collect function names
     */
    virtual FArchive& operator<<(FName &Value) override
    {
        FunctionNames.Add(Value);
        return *this;
    }

    TArray<UObject*> Objects;
    TArray<FName> FunctionNames;
};

void FSignatureDesc::MarkForDelete(bool bIgnoreBindings)
{
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

void FSignatureDesc::Execute(UObject *Context, FFrame &Stack, void *RetValueAddress)
{
    if (SignatureFunctionDesc)
    {
        ++NumCalls;         // inc calls, so it won't be deleted during call
        SignatureFunctionDesc->CallLua(Context, Stack, RetValueAddress, false, false);
        --NumCalls;         // dec calls
        if (!NumCalls && bPendingKill)
        {
            check(NumBindings == 1);
            FDelegateHelper::CleanUpByFunction(SignatureFunctionDesc->GetFunction());       // clean up the delegate only if there is only one bindings.
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

DEFINE_FUNCTION(FDelegateHelper::ProcessDelegate)
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    FSignatureDesc *SignatureDesc = nullptr;
    FMemory::Memcpy(&SignatureDesc, Stack.Code, sizeof(SignatureDesc));
    //Stack.SkipCode(sizeof(SignatureDesc));        // skip 'FSignatureDesc' pointer
#else
    FSignatureDesc **SignatureDescPtr = Signatures.Find(Stack.CurrentNativeFunction);   // find the signature
    FSignatureDesc *SignatureDesc = SignatureDescPtr ? *SignatureDescPtr : nullptr;
#endif
    if (SignatureDesc)
    {
        SignatureDesc->Execute(Context, Stack, (void*)RESULT_PARAM);     // fire the delegate
        return;
    }
    UE_LOG(LogUnLua, Warning, TEXT("Failed to process delegate (%s)!"), *Stack.CurrentNativeFunction->GetName());
}

FName FDelegateHelper::GetBindedFunctionName(const FCallbackDesc &Callback)
{
    UFunction **CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        FSignatureDesc **SignatureDesc = Signatures.Find(*CallbackFuncPtr);
        ++((*SignatureDesc)->NumBindings);          // inc bindings
        return (*CallbackFuncPtr)->GetFName();      // return function name
    }
    return NAME_None;
}

void FDelegateHelper::PreBind(FScriptDelegate *ScriptDelegate, FDelegateProperty *Property)
{
    check(ScriptDelegate && Property);
    FDelegateProperty **PropertyPtr = Delegate2Property.Find(ScriptDelegate);
    if (!PropertyPtr)
    {
        Delegate2Property.Add(ScriptDelegate, Property);
    }
}

bool FDelegateHelper::Bind(FScriptDelegate *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef)
{
    FDelegateProperty **PropertyPtr = Delegate2Property.Find(ScriptDelegate);
    return PropertyPtr ? Bind(ScriptDelegate, *PropertyPtr, Object, Callback, CallbackRef) : false;
}

bool FDelegateHelper::Bind(FScriptDelegate *ScriptDelegate, FDelegateProperty *Property, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef)
{
    if (!ScriptDelegate || ScriptDelegate->IsBound() || !Property || !Object || !Callback.Class || CallbackRef == INDEX_NONE)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid Delegate Bind! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

    UFunction **CallbackFuncPtr = Callbacks.Find(Callback);
    if (!CallbackFuncPtr)
    {
        FName FuncName(*FString::Printf(TEXT("%s_%s"), *Property->GetName(), *FGuid::NewGuid().ToString()));
        ScriptDelegate->BindUFunction(Object, FuncName);                                    // bind a callback to the delegate
        CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback
    }
    return true;
}

void FDelegateHelper::Unbind(const FCallbackDesc &Callback)
{
    UFunction **CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        FSignatureDesc **SignatureDesc = Signatures.Find(*CallbackFuncPtr);
        if (SignatureDesc && *SignatureDesc)
        {
            (*SignatureDesc)->MarkForDelete(true);
        }
    }
}

void FDelegateHelper::Unbind(FScriptDelegate *ScriptDelegate)
{
    check(ScriptDelegate);
    if (!ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s, FScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    UObject *Object = ScriptDelegate->GetUObject();
    if (Object)
    {
        UFunction *Function = Object->FindFunction(ScriptDelegate->GetFunctionName());
        if (Function)
        {
            // try to delete the signature
            FSignatureDesc **SignatureDesc = Signatures.Find(Function);
            if (SignatureDesc && *SignatureDesc)
            {
                (*SignatureDesc)->MarkForDelete();
            }
        }
    }

    Delegate2Property.Remove(ScriptDelegate);
    ScriptDelegate->Unbind();       // unbind the delegate
}

int32 FDelegateHelper::Execute(lua_State *L, FScriptDelegate *ScriptDelegate, int32 NumParams, int32 FirstParamIndex)
{
    check(ScriptDelegate);
    if (!ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: FScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FFunctionDesc *SignatureFunctionDesc = nullptr;
    FFunctionDesc **SignatureFunctionDescPtr = Delegate2Signatures.Find(ScriptDelegate);
    if (SignatureFunctionDescPtr)
    {
        SignatureFunctionDesc = *SignatureFunctionDescPtr;
    }
    else
    {
        FDelegateProperty **PropertyPtr = Delegate2Property.Find(ScriptDelegate);
        if (PropertyPtr)
        {
            UFunction *SignatureFunction = (*PropertyPtr)->SignatureFunction;
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

void FDelegateHelper::PreAdd(FMulticastDelegateType *ScriptDelegate, FMulticastDelegateProperty *Property)
{
    check(ScriptDelegate && Property);
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if (!PropertyPtr)
    {
        MulticastDelegate2Property.Add(ScriptDelegate, Property);
    }
}

bool FDelegateHelper::Add(FMulticastDelegateType *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef)
{
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    return PropertyPtr ? Add(ScriptDelegate, *PropertyPtr, Object, Callback, CallbackRef) : false;
}

bool FDelegateHelper::Add(FMulticastDelegateType *ScriptDelegate, FMulticastDelegateProperty *Property, UObject *Object, const FCallbackDesc &Callback, int32 CallbackRef)
{
    if (!ScriptDelegate || !Property || !Object || !Callback.Class || CallbackRef == INDEX_NONE)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid Delegate Add! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

    UFunction **CallbackFuncPtr = Callbacks.Find(Callback);
    if (!CallbackFuncPtr)
    {
        FName FuncName(*FString::Printf(TEXT("%s_%s"), *Property->GetName(), *FGuid::NewGuid().ToString()));
        FScriptDelegate DynamicDelegate;
        DynamicDelegate.BindUFunction(Object, FuncName);
        CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback
        TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // add a callback to the delegate
    }
    return true;
}

void FDelegateHelper::Remove(FMulticastDelegateType *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback)
{
    check(ScriptDelegate && Object);
#if ENGINE_MINOR_VERSION < 23           // todo: how to check it for 4.23.x and above...
    if (!ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s, multicast delegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }
#endif

    UFunction **CallbackFuncPtr = Callbacks.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        FMulticastDelegateProperty *Property = nullptr;
        MulticastDelegate2Property.RemoveAndCopyValue(ScriptDelegate, Property);
        if (!Property)
        {
            return;
        }

        FMulticastScriptDelegate *MulticastDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, ScriptDelegate);
        if (!MulticastDelegate)
        {
            return;
        }

        FScriptDelegate DynamicDelegate;
        DynamicDelegate.BindUFunction(Object, (*CallbackFuncPtr)->GetFName());
        if (MulticastDelegate->Contains(DynamicDelegate))
        {
            TMulticastDelegateTraits<FMulticastDelegateType>::RemoveDelegate(Property, DynamicDelegate, ScriptDelegate);    // remove a callback from the delegate

            // try to delete the signature
            FSignatureDesc **SignatureDesc = Signatures.Find(*CallbackFuncPtr);
            if (SignatureDesc && *SignatureDesc)
            {
                (*SignatureDesc)->MarkForDelete();
            }
        }
    }
}

void FDelegateHelper::Clear(FMulticastDelegateType *InScriptDelegate)
{
    if (!InScriptDelegate /*|| !InScriptDelegate->IsBound()*/)
    {
        return;
    }

    FMulticastDelegateProperty *Property = nullptr;
#if ENGINE_MINOR_VERSION > 22
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(InScriptDelegate);
    if (!PropertyPtr || !(*PropertyPtr))
    {
        return;     // invalid FMulticastDelegateProperty
    }
    Property = *PropertyPtr;
#endif
    FMulticastScriptDelegate *ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, InScriptDelegate);  // get target delegate

    // get all binded callbacks
    FInvocationListReader InvocationList;
    InvocationList << (*ScriptDelegate);
    check(InvocationList.Objects.Num() == InvocationList.FunctionNames.Num());

    for (int32 i = 0; i < InvocationList.Objects.Num(); ++i)
    {
        if (InvocationList.Objects[i])
        {
            UFunction *Function = InvocationList.Objects[i]->FindFunction(InvocationList.FunctionNames[i]);
            if (Function)
            {
                // try to delete the signature
                FSignatureDesc **SignatureDesc = Signatures.Find(Function);
                if (SignatureDesc && *SignatureDesc)
                {
                    (*SignatureDesc)->MarkForDelete();
                }
            }
        }
    }

    TMulticastDelegateTraits<FMulticastDelegateType>::ClearDelegate(Property, InScriptDelegate);        // clear all callbacks
}

void FDelegateHelper::Broadcast(lua_State *L, FMulticastDelegateType *InScriptDelegate, int32 NumParams, int32 FirstParamIndex)
{
    check(InScriptDelegate);
#if ENGINE_MINOR_VERSION < 23           // todo: how to check it for 4.23.x and above...
    if (!InScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: multicast delegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }
#endif

    FMulticastDelegateProperty *Property = nullptr;
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(InScriptDelegate);
    if (PropertyPtr)
    {
        Property = *PropertyPtr;
    }

    FFunctionDesc *SignatureFunctionDesc = nullptr;
    FFunctionDesc **SignatureFunctionDescPtr = MulticastDelegate2Signatures.Find(InScriptDelegate);
    if (SignatureFunctionDescPtr)
    {
        SignatureFunctionDesc = *SignatureFunctionDescPtr;
    }
    else
    {
        UFunction *SignatureFunction = Property->SignatureFunction;
        SignatureFunctionDesc = GReflectionRegistry.RegisterFunction(SignatureFunction);
        MulticastDelegate2Signatures.Add(InScriptDelegate, SignatureFunctionDesc);
    }

    if (SignatureFunctionDesc && Property)
    {
        FMulticastScriptDelegate *ScriptDelegate = TMulticastDelegateTraits<FMulticastDelegateType>::GetMulticastDelegate(Property, InScriptDelegate);  // get target delegate
        SignatureFunctionDesc->BroadcastMulticastDelegate(L, NumParams, FirstParamIndex, ScriptDelegate);        // fire the delegate
        return;
    }

    UE_LOG(LogUnLua, Warning, TEXT("Failed to broadcast multicast delegate!!!"));
}

void FDelegateHelper::AddDelegate(FMulticastDelegateType *ScriptDelegate, FScriptDelegate DynamicDelegate)
{
    FMulticastDelegateProperty *Property = nullptr;
#if ENGINE_MINOR_VERSION > 22
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if (!PropertyPtr || !(*PropertyPtr))
    {
        return;     // invalid FMulticastDelegateProperty
    }
    Property = *PropertyPtr;
#endif
    TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // adds a callback to delegate's invocation list
}

void FDelegateHelper::CleanUpByFunction(UFunction *Function)
{
    // cleanup all associated stuff of a UFunction
    FCallbackDesc Callback;
    if (Function2Callback.RemoveAndCopyValue(Function, Callback))
    {
        Callbacks.Remove(Callback);

        FSignatureDesc *SignatureDesc = nullptr;
        if (Signatures.RemoveAndCopyValue(Function, SignatureDesc))
        {
            delete SignatureDesc;
        }

        TArray<UFunction*> *FunctionsPtr = Class2Functions.Find(Callback.Class);
        if (FunctionsPtr)
        {
            FunctionsPtr->Remove(Function);
            if (FunctionsPtr->Num() < 1)
            {
                Class2Functions.Remove(Callback.Class);
            }
        }

        RemoveUFunction(Function, Callback.Class);      // remove the duplicated function
    }
}

void FDelegateHelper::CleanUpByClass(UClass *Class)
{
    // cleanup all associated stuff of a UClass
    TArray<UFunction*> Functions;
    if (Class2Functions.RemoveAndCopyValue(Class, Functions))
    {
        for (UFunction *Function : Functions)
        {
            RemoveUFunction(Function, Class);           // remove the duplicated function

            FSignatureDesc *SignatureDesc = nullptr;
            if (Signatures.RemoveAndCopyValue(Function, SignatureDesc))
            {
                delete SignatureDesc;
            }

            FCallbackDesc Callback;
            if (Function2Callback.RemoveAndCopyValue(Function, Callback))
            {
                Callbacks.Remove(Callback);
            }
        }
    }
}

void FDelegateHelper::Cleanup(bool bFullCleanup)
{
    // cleanup all stuff during level transition
    if (bFullCleanup)
    {
        for (TMap<UClass*, TArray<UFunction*>>::TIterator It(Class2Functions); It; ++It)
        {
            UClass *Class = It.Key();
            TArray<UFunction*> &Functions = It.Value();
            for (UFunction *Function : Functions)
            {
                RemoveUFunction(Function, Class);       // remove the duplicated function
            }
        }
        Class2Functions.Empty();

        for (TMap<UFunction*, FSignatureDesc*>::TIterator It(Signatures); It; ++It)
        {
            delete It.Value();
        }
        Signatures.Empty();

        Callbacks.Empty();
        Function2Callback.Empty();
    }

    Delegate2Property.Empty();
    MulticastDelegate2Property.Empty();

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
}

/**
 * 1. Create a new signature UFunction
 * 2. Set a custom thunk function for the new signature
 * 3. Create a signature descriptor
 * 4. Update function flags for the new signature if necessary
 * 5. Update cached infos
 */
void FDelegateHelper::CreateSignature(UFunction *TemplateFunction, FName FuncName, const FCallbackDesc &Callback, int32 CallbackRef)
{
    UFunction *SignatureFunction = DuplicateUFunction(TemplateFunction, Callback.Class, FuncName);      // duplicate the signature UFunction
    SignatureFunction->Script.Empty();

    FSignatureDesc *SignatureDesc = new FSignatureDesc;
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

    TArray<UFunction*> &Functions = Class2Functions.FindOrAdd(Callback.Class);
    Functions.Add(SignatureFunction);
}
