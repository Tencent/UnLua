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

void FSignatureDesc::MarkForDelete(bool bIgnoreBindings, UObject* Object)
{
	if (!bIgnoreBindings && NumBindings > 1)
	{
		--NumBindings;              // dec bindings if there is more than one bindings.
		UE_LOG(UnLuaDelegate, Verbose, TEXT("-- %d %s %p %s"), NumBindings, Object ? *Object->GetName() : TEXT("nullptr"), Object, *SignatureFunctionDesc->GetFunction()->GetName());
		return;
	}
	else if (NumCalls > 0)
	{
		UE_LOG(UnLuaDelegate, Verbose, TEXT("bPendingKill %d %s %p %s"), NumBindings, Object ? *Object->GetName() : TEXT("nullptr"), Object, *SignatureFunctionDesc->GetFunction()->GetName());
		bPendingKill = true;        // raise pending kill flag if it's being called
		return;
	}

	UE_LOG(UnLuaDelegate, Verbose, TEXT("Clean %d %s %p %s"), NumBindings, Object ? *Object->GetName() : TEXT("nullptr"), Object, *SignatureFunctionDesc->GetFunction()->GetName());
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
            if (NumBindings > 1) // NumBindings may increase when running CallLua
            {
                bPendingKill = false;
				UE_LOG(UnLuaDelegate, Verbose, TEXT("++ again after --, cannot kill dele, %d %s %p %s"), NumBindings, Context ? *Context->GetName() : TEXT("nullptr"), Context, *SignatureFunctionDesc->GetFunction()->GetName());
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

TMap<UFunction*, FSignatureDesc*> FDelegateHelper::Function2Signature;
TMap<FCallbackDesc, UFunction*> FDelegateHelper::Callback2Function;
TMap<UFunction*, FCallbackDesc> FDelegateHelper::Function2Callback;
TMap<UClass*, TArray<UFunction*>> FDelegateHelper::Class2Functions;

TMap<FMulticastDelegateType*, TArray<FCallbackDesc>> FDelegateHelper::MutiDelegates2Callback;

DEFINE_FUNCTION(FDelegateHelper::ProcessDelegate)
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    FSignatureDesc *SignatureDesc = nullptr;
    FMemory::Memcpy(&SignatureDesc, Stack.Code, sizeof(SignatureDesc));
    //Stack.SkipCode(sizeof(SignatureDesc));        // skip 'FSignatureDesc' pointer
#else
    FSignatureDesc **SignatureDescPtr = Function2Signature.Find(Stack.CurrentNativeFunction);   // find the signature
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
    UFunction **CallbackFuncPtr = Callback2Function.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {
        FSignatureDesc **SignatureDesc = Function2Signature.Find(*CallbackFuncPtr);
        ++((*SignatureDesc)->NumBindings);          // inc bindings
        return (*CallbackFuncPtr)->GetFName();      // return function name
    }
    return NAME_None;
}

int32 FDelegateHelper::GetNumBindings(const FCallbackDesc& Callback)
{
	UFunction** CallbackFuncPtr = Callback2Function.Find(Callback);
	if (CallbackFuncPtr && *CallbackFuncPtr)
	{
		FSignatureDesc** SignatureDesc = Function2Signature.Find(*CallbackFuncPtr);
        return (*SignatureDesc)->NumBindings;
	}
	return -1;
}

void FDelegateHelper::PreBind(FScriptDelegate *ScriptDelegate, FDelegateProperty *Property)
{
    check(ScriptDelegate && Property);
    FDelegateProperty **PropertyPtr = Delegate2Property.Find(ScriptDelegate);
    if ((!PropertyPtr)
        || (*PropertyPtr != Property))
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
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Delegate Bind! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

	UFunction** CallbackFuncPtr = Callback2Function.Find(Callback);
	if (!CallbackFuncPtr)
	{

		lua_State* L = UnLua::GetState();
		lua_Debug ar;

		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.linedefined;
		auto name = ar.source;

		FName FuncName(*FString::Printf(TEXT("LuaFunc:[%s:%d]_CppDelegate:[%s.%s_%s]"), ANSI_TO_TCHAR(name), line, *Object->GetName(), *Property->GetName(), *FGuid::NewGuid().ToString()));

		UE_LOG(UnLuaDelegate, Verbose, TEXT("++ 1 %s %p %s"), *Object->GetName(), Object, *FuncName.ToString());
        ScriptDelegate->BindUFunction(Object, FuncName);                                    // bind a callback to the delegate
		CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback
	}
	return true;
}

void FDelegateHelper::Unbind(const FCallbackDesc &Callback)
{
	UFunction** FunctionPtr = Callback2Function.Find(Callback);
	if (FunctionPtr && *FunctionPtr)
	{
        UFunction* Function = *FunctionPtr;
        UObject* Object = Callback.Object;

		// try to delete the signature
		FSignatureDesc** SignatureDesc = Function2Signature.Find(Function);
		if (SignatureDesc && *SignatureDesc)
		{
			(*SignatureDesc)->MarkForDelete();
		}
	}
}

void FDelegateHelper::Unbind(FScriptDelegate *ScriptDelegate)
{
    check(ScriptDelegate);
    if (!ScriptDelegate->IsBound())
    {
        return;
    }

    UObject *Object = ScriptDelegate->GetUObject();
    if (Object)
    {
        UFunction *Function = Object->FindFunction(ScriptDelegate->GetFunctionName());
        if (Function)
        {
            // try to delete the signature
            FSignatureDesc **SignatureDesc = Function2Signature.Find(Function);
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

void FDelegateHelper::PreAdd(FMulticastDelegateType* ScriptDelegate, FMulticastDelegateProperty* Property)
{
    check(ScriptDelegate && Property);
    FMulticastDelegateProperty** PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if ((!PropertyPtr)
         ||((*PropertyPtr) != Property))
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
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Delegate Add! : %s"), ANSI_TO_TCHAR(__FUNCTION__), *(Property->GetName()));
        return false;
    }

#if UNLUA_ENABLE_DEBUG != 0
    UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Add: %p,%p,%s"), ScriptDelegate, Object, *Object->GetName());
#endif
    
	UFunction** CallbackFuncPtr = Callback2Function.Find(Callback);
	if (!CallbackFuncPtr)
	{
		lua_State* L = UnLua::GetState();
		lua_Debug ar;

		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.linedefined;
		auto name = ar.source;

		FName FuncName(*FString::Printf(TEXT("LuaFunc:[%s:%d]_CppMulticastDelegate:[%s.%s_%s]"), ANSI_TO_TCHAR(name), line, *Object->GetName(), *Property->GetName(), *FGuid::NewGuid().ToString()));

        FScriptDelegate DynamicDelegate;
		DynamicDelegate.BindUFunction(Object, FuncName);

		UE_LOG(UnLuaDelegate, Verbose, TEXT("++ 1 %s %p %s"), *Object->GetName(), Object, *FuncName.ToString());

		CreateSignature(Property->SignatureFunction, FuncName, Callback, CallbackRef);      // create the signature function for the callback
		TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // add a callback to the delegate

		TArray<FCallbackDesc>& DelegateCallbacks = MutiDelegates2Callback.FindOrAdd(ScriptDelegate);
		DelegateCallbacks.AddUnique(Callback);
	}
    return true;
}

void FDelegateHelper::Remove(FMulticastDelegateType *ScriptDelegate, UObject *Object, const FCallbackDesc &Callback)
{
    check(ScriptDelegate && Object);

    UFunction** CallbackFuncPtr = Callback2Function.Find(Callback);
    if (CallbackFuncPtr && *CallbackFuncPtr)
    {

        if (GLuaCxt->IsUObjectValid((UObjectBase*)Object))
        {
            FMulticastDelegateProperty** Property = MulticastDelegate2Property.Find(ScriptDelegate);
            if ((!Property)
                ||(!*Property))
            {
                return;
            }

            FPropertyDesc** PropertyDesc = FPropertyDesc::Property2Desc.Find(*Property);
            if ((!PropertyDesc)
                ||(!GReflectionRegistry.IsDescValid(*PropertyDesc,DESC_PROPERTY)))
            {
                return;
            }

#if UNLUA_ENABLE_DEBUG != 0
            UE_LOG(LogUnLua, Log, TEXT("FDelegateHelper::Remove: %p,%p,%s"), ScriptDelegate, Object, *Object->GetName());
#endif

            FScriptDelegate DynamicDelegate;
            DynamicDelegate.BindUFunction(Object, (*CallbackFuncPtr)->GetFName());
            TMulticastDelegateTraits<FMulticastDelegateType>::RemoveDelegate(*Property, DynamicDelegate, ScriptDelegate);    // remove a callback from the delegate
            
            // try to delete the signature
            FSignatureDesc** SignatureDesc = Function2Signature.Find(*CallbackFuncPtr);
            if (SignatureDesc && *SignatureDesc)
            {
                (*SignatureDesc)->MarkForDelete(false, Object);
            }
        }
    }
}

void FDelegateHelper::Remove(UObject* Object)
{
	TArray<UFunction*> FunctionsPtr = Class2Functions.FindRef(Object->GetClass());
	for (int i = 0; i < FunctionsPtr.Num(); ++i)
	{
		FCallbackDesc callback = Function2Callback.FindRef(FunctionsPtr[i]);
		if (callback.Object == Object)
		{
			FSignatureDesc* SignatureDesc = Function2Signature.FindRef(FunctionsPtr[i]);
			if (SignatureDesc)
			{
				SignatureDesc->MarkForDelete(true);
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
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(InScriptDelegate);
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
            UFunction** CallbackFuncPtr = Callback2Function.Find(Callback);
            if (CallbackFuncPtr && *CallbackFuncPtr)
            {
                FSignatureDesc** SignatureDesc = Function2Signature.Find(*CallbackFuncPtr);
                if (SignatureDesc && *SignatureDesc)
                {
                    (*SignatureDesc)->MarkForDelete();
                }
            }
        }
    }
}

void FDelegateHelper::Broadcast(lua_State *L, FMulticastDelegateType *InScriptDelegate, int32 NumParams, int32 FirstParamIndex)
{
    check(InScriptDelegate);
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 23           // todo: how to check it for 4.23.x and above...
    if (!InScriptDelegate->IsBound())
    {
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

void FDelegateHelper::AddDelegate(FMulticastDelegateType *ScriptDelegate, UObject* Object, const FCallbackDesc& Callback,FScriptDelegate DynamicDelegate)
{
    FMulticastDelegateProperty *Property = nullptr;
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
    FMulticastDelegateProperty **PropertyPtr = MulticastDelegate2Property.Find(ScriptDelegate);
    if (!PropertyPtr || !(*PropertyPtr))
    {
        return;     // invalid FMulticastDelegateProperty
    }
    Property = *PropertyPtr;
#endif
    TMulticastDelegateTraits<FMulticastDelegateType>::AddDelegate(Property, DynamicDelegate, ScriptDelegate);   // adds a callback to delegate's invocation list

    TArray<FCallbackDesc>& DelegateCallbacks = MutiDelegates2Callback.FindOrAdd(ScriptDelegate);
    DelegateCallbacks.AddUnique(Callback);
}

void FDelegateHelper::CleanUpByFunction(UFunction *Function)
{
    // cleanup all associated stuff of a UFunction
    FSignatureDesc* SignatureDesc = nullptr;
    if (Function2Signature.RemoveAndCopyValue(Function, SignatureDesc))
    {
        delete SignatureDesc;
    }

    FCallbackDesc Callback;
    if (Function2Callback.RemoveAndCopyValue(Function, Callback))
    {
        Callback2Function.Remove(Callback);

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

void FDelegateHelper::CleanUpByClass(UClass *Class)
{
    // cleanup all associated stuff of a UClass
    TArray<UFunction*> Functions;
    if (Class2Functions.RemoveAndCopyValue(Class, Functions))
    {
        for (UFunction *Function : Functions)
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
    Function2Signature.Empty();
    Callback2Function.Empty();
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
    MutiDelegates2Callback.Empty();
}

void FDelegateHelper::NotifyUObjectDeleted(UObject* InObject)
{   
    Remove(InObject);
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
    Function2Signature.Add(SignatureFunction, SignatureDesc);

    OverrideUFunction(SignatureFunction, (FNativeFuncPtr)&FDelegateHelper::ProcessDelegate, SignatureDesc, false);      // set custom thunk function for the duplicated UFunction

    uint8 NumRefProperties = SignatureDesc->SignatureFunctionDesc->GetNumRefProperties();
    if (NumRefProperties > 0)
    {
        SignatureFunction->FunctionFlags |= FUNC_HasOutParms;        // 'FUNC_HasOutParms' will not be set for signature function even if it has out parameters
    }

    Callback2Function.Add(Callback, SignatureFunction);
    Function2Callback.Add(SignatureFunction, Callback);

    TArray<UFunction*> &Functions = Class2Functions.FindOrAdd(Callback.Class);
    Functions.Add(SignatureFunction);
}