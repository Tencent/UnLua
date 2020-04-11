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

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "Engine/UserDefinedEnum.h"
#include "UnLuaBase.h"
#include "UnLuaCompatibility.h"

#define ENABLE_PERSISTENT_PARAM_BUFFER 1            // option to allocate persistent buffer for UFunction's parameters
#define ENABLE_CALL_OVERRIDDEN_FUNCTION 1           // option to call overridden UFunction

/**
 * new UProperty types
 */
enum
{
#if ENGINE_MINOR_VERSION > 22
    CPT_MulticastSparseDelegate = CPT_Unused_Index_19,
#endif
    CPT_Enum = CPT_Unused_Index_21,
    CPT_Array = CPT_Unused_Index_22,
};

struct lua_State;

class FFieldDesc;
class FClassDesc;

/**
 * Property descriptor
 */
class FPropertyDesc : public UnLua::ITypeInterface
{
public:
    static FPropertyDesc* Create(UProperty *InProperty);

    virtual ~FPropertyDesc() {}

    /**
     * Check the validity of this property
     *
     * @return - true if the property is valid, false otherwise
     */
    FORCEINLINE bool IsValid() const  { return Property != nullptr; }

    /**
     * Test if this property is a const reference parameter.
     *
     * @return - true if the property is a const reference parameter, false otherwise
     */
    FORCEINLINE bool IsConstOutParameter() const { return Property->HasAllPropertyFlags(CPF_OutParm | CPF_ConstParm); }

    /**
     * Test if this property is a non-const reference parameter.
     *
     * @return - true if the property is a non-const reference parameter, false otherwise
     */
    FORCEINLINE bool IsNonConstOutParameter() const { return Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm); }

    /**
     * Test if this property is an out parameter. out parameter means return parameter or non-const reference parameter
     *
     * @return - true if the property is an out parameter, false otherwise
     */
    FORCEINLINE bool IsOutParameter() const { return Property->HasAnyPropertyFlags(CPF_ReturnParm) || (Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm)); }

    /**
     * Test if this property is the return parameter
     *
     * @return - true if the property is the return parameter, false otherwise
     */
    FORCEINLINE bool IsReturnParameter() const { return Property->HasAnyPropertyFlags(CPF_ReturnParm); }

    /**
     * Get the 'true' property
     *
     * @return - the UProperty
     */
    FORCEINLINE UProperty* GetProperty() const { return Property; }

    /**
     * @see UProperty::InitializeValue_InContainer(...)
     */
    FORCEINLINE void InitializeValue(void *ContainerPtr) const { Property->InitializeValue_InContainer(ContainerPtr); }

    /**
     * @see UProperty::DestroyValue_InContainer(...)
     */
    FORCEINLINE void DestroyValue(void *ContainerPtr) const { Property->DestroyValue_InContainer(ContainerPtr); }

    /**
     * @see UProperty::CopySingleValue(...)
     */
    FORCEINLINE void CopyValue(void *ContainerPtr, const void *Src) const { Property->CopySingleValue(Property->ContainerPtrToValuePtr<void>(ContainerPtr), Src); }

    /**
     * Get the value of this property
     *
     * @param ContainerPtr - the address of the container for this property
     * @param bCreateCopy (Optional) - whether to create a copy for the value
     */
    FORCEINLINE void GetValue(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const { GetValueInternal(L, Property->ContainerPtrToValuePtr<void>(ContainerPtr), bCreateCopy); }

    /**
     * Set the value of this property as an element at the given Lua index
     *
     * @param ContainerPtr - the address of the container for this property
     * @param IndexInStack - Lua index
     * @param bCopyValue - whether to create a copy for the value
     * @return - true if 'ContainerPtr' should be cleaned up by 'DestroyValue_InContainer', false otherwise
     */
    FORCEINLINE bool SetValue(lua_State *L, void *ContainerPtr, int32 IndexInStack = -1, bool bCopyValue = true) const
    {
        return SetValueInternal(L, Property->ContainerPtrToValuePtr<void>(ContainerPtr), IndexInStack, bCopyValue);
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const = 0;
    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack = -1, bool bCopyValue = true) const = 0;

    /**
     * Copy an element at the given Lua index to the value of this property
     *
     * @param SrcIndexInStack - source Lua index
     * @param DestContainerPtr - the destination address of the container for this property
     * @return - true if the operation succeed, false otherwise
     */
    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) { return false; }

    /**
     * Copy the value of this property to an element at the given Lua index
     *
     * @param SrcContainerPtr - the source address of the container for this property
     * @param DestIndexInStack - destination Lua index
     * @return - true if the operation succeed, false otherwise
     */
    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) { return false; }

    virtual bool CopyBack(void *Dest, const void *Src) { return false; }

    // interfaces of UnLua::ITypeInterface
    virtual bool IsPODType() const override { return (Property->PropertyFlags & CPF_IsPlainOldData) != 0; }
    virtual bool IsTriviallyDestructible() const override { return (Property->PropertyFlags & CPF_NoDestructor) != 0; }
    virtual int32 GetOffset() const override { return Property->GetOffset_ForInternal(); }
    virtual int32 GetSize() const override { return Property->GetSize(); }
    virtual int32 GetAlignment() const override { return Property->GetMinAlignment(); }
    virtual uint32 GetValueTypeHash(const void *Src) const override { return Property->GetValueTypeHash(Src); }
    virtual void Initialize(void *Dest) const override { Property->InitializeValue(Dest); }
    virtual void Destruct(void *Dest) const override { Property->DestroyValue(Dest); }
    virtual void Copy(void *Dest, const void *Src) const override { Property->CopySingleValue(Dest, Src); }
    virtual bool Identical(const void *A, const void *B) const override { return Property->Identical(A, B); }
    virtual FString GetName() const override { return TEXT(""); }
    virtual UProperty* GetUProperty() const override { return Property; }
    virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override { GetValueInternal(L, Property->ContainerPtrToValuePtr<void>(ContainerPtr), bCreateCopy); }
    virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override { SetValueInternal(L, Property->ContainerPtrToValuePtr<void>(ContainerPtr), IndexInStack, true); }

protected:
    explicit FPropertyDesc(UProperty *InProperty) : Property(InProperty) {}

    union
    {
        UProperty *Property;
        UNumericProperty *NumericProperty;
        UEnumProperty *EnumProperty;
        UBoolProperty *BoolProperty;
        UObjectPropertyBase *ObjectBaseProperty;
        USoftObjectProperty *SoftObjectProperty;
        UInterfaceProperty *InterfaceProperty;
        UNameProperty *NameProperty;
        UStrProperty *StringProperty;
        UTextProperty *TextProperty;
        UArrayProperty *ArrayProperty;
        UMapProperty *MapProperty;
        USetProperty *SetProperty;
        UStructProperty *StructProperty;
        UDelegateProperty *DelegateProperty;
        UMulticastDelegateProperty *MulticastDelegateProperty;
    };
};

/**
 * Function descriptor
 */
class FFunctionDesc
{
    friend class FReflectionRegistry;

public:
    FFunctionDesc(UFunction *InFunction, struct FParameterCollection *InDefaultParams, int32 InFunctionRef = INDEX_NONE);
    ~FFunctionDesc();

    /**
     * Check the validity of this function
     *
     * @return - true if the function is valid, false otherwise
     */
    FORCEINLINE bool IsValid() const { return Function != nullptr; }

    /**
     * Test if this function has return property
     *
     * @return - true if the function has return property, false otherwise
     */
    FORCEINLINE bool HasReturnProperty() const { return ReturnPropertyIndex > INDEX_NONE; }

    /**
     * Test if this function is a latent function
     *
     * @return - true if the function is a latent function, false otherwise
     */
    FORCEINLINE bool IsLatentFunction() const { return LatentPropertyIndex > INDEX_NONE; }

    /**
     * Get the number of properties (includes both in and out properties)
     *
     * @return - the number of properties
     */
    FORCEINLINE uint8 GetNumProperties() const { return Function->NumParms; }

    /**
     * Get the number of out properties
     *
     * @return - the number of out properties. out properties means return property or non-const reference properties
     */
    FORCEINLINE uint8 GetNumOutProperties() const { return ReturnPropertyIndex > INDEX_NONE ? OutPropertyIndices.Num() + 1 : OutPropertyIndices.Num(); }

    /**
     * Get the number of reference properties
     *
     * @return - the number of reference properties.
     */
    FORCEINLINE uint8 GetNumRefProperties() const { return NumRefProperties; }

    /**
     * Get the number of non-const reference properties
     *
     * @return - the number of non-const reference properties.
     */
    FORCEINLINE uint8 GetNumNoConstRefProperties() const { return OutPropertyIndices.Num(); }

    /**
     * Get the 'true' function
     *
     * @return - the UFunction
     */
    FORCEINLINE UFunction* GetFunction() const { return Function; }

    /**
     * Call Lua function that overrides this UFunction
     *
     * @param Stack - script execution stack
     * @param RetValueAddress - address of return value
     * @param bRpcCall - whether this function is a RPC function
     * @param bUnpackParams - whether to unpack parameters from the stack
     * @return - true if the Lua function executes successfully, false otherwise
     */
    bool CallLua(FFrame &Stack, void *RetValueAddress, bool bRpcCall, bool bUnpackParams);

    /**
     * Call this UFunction
     *
     * @param NumParams - the number of parameters
     * @param Userdata - user data, now it's only used for latent function and it must be a 'int32'
     * @return - the number of return values pushed on the stack
     */
    int32 CallUE(lua_State *L, int32 NumParams, void *Userdata = nullptr);

    /**
     * Fire the delegate
     *
     * @param NumParams - the number of parameters
     * @param FirstParamIndex - Lua index of the first parameter
     * @param ScriptDelegate - the delegate
     * @return - the number of return values pushed on the stack
     */
    int32 ExecuteDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FScriptDelegate *ScriptDelegate);

    /**
     * Fire the multicast delegate
     *
     * @param NumParams - the number of parameters
     * @param FirstParamIndex - Lua index of the first parameter
     * @param ScriptDelegate - the multicast delegate
     */
    void BroadcastMulticastDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FMulticastScriptDelegate *ScriptDelegate);

private:
    void* PreCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, TArray<bool> &CleanupFlags, void *Userdata = nullptr);
    int32 PostCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, void *Params, const TArray<bool> &CleanupFlags);

    bool CallLuaInternal(lua_State *L, void *InParams, FOutParmRec *OutParams, void *RetValueAddress) const;

    UFunction *Function;
    FString FuncName;
#if ENABLE_PERSISTENT_PARAM_BUFFER
    void *Buffer;
#endif
#if !SUPPORTS_RPC_CALL
    FOutParmRec *OutParmRec;
#endif
    TArray<FPropertyDesc*> Properties;
    TArray<int32> OutPropertyIndices;
    FParameterCollection *DefaultParams;
    int32 ReturnPropertyIndex;
    int32 LatentPropertyIndex;
    int32 FunctionRef;
    uint8 NumRefProperties;
    uint8 NumCalls;                 // RECURSE_LIMIT is 120 or 250 which is less than 256, so use a byte...
    uint8 bStaticFunc : 1;
    uint8 bInterfaceFunc : 1;
};

/**
 * Class descriptor
 */
class FClassDesc
{
    friend class FReflectionRegistry;

public:
    enum class EType
    {
        SCRIPTSTRUCT,
        CLASS,
        UNKNOWN
    };

    FClassDesc(UStruct *InStruct, const FString &InName, EType InType);
    ~FClassDesc();

    FORCEINLINE bool IsValid() const { return Type != EType::UNKNOWN && Struct/* && Struct->IsValidLowLevel()*/; }

    FORCEINLINE bool IsScriptStruct() const { return Type == EType::SCRIPTSTRUCT; }

    FORCEINLINE bool IsClass() const { return Type == EType::CLASS; }

    FORCEINLINE bool IsInterface() const { return Type == EType::CLASS && Class->HasAnyClassFlags(CLASS_Interface) && Class != UInterface::StaticClass(); }

    FORCEINLINE bool IsNative() const { return Struct->IsNative(); }

    FORCEINLINE UStruct* AsStruct() const { return Struct; }

    FORCEINLINE UScriptStruct* AsScriptStruct() const { return Type == EType::SCRIPTSTRUCT ? ScriptStruct : nullptr; }

    FORCEINLINE UClass* AsClass() const { return Type == EType::CLASS ? Class : nullptr; }

    FORCEINLINE FClassDesc* GetParent() const { return Parent; }

    FORCEINLINE const FString& GetName() const { return ClassName; }

    FORCEINLINE FName GetFName() const { return ClassFName; }

    FORCEINLINE const char* GetAnsiName() const { return ClassAnsiName.Get(); }

    FORCEINLINE int32 GetSize() const { return Size; }

    FORCEINLINE uint8 GetUserdataPadding() const { return UserdataPadding; }

    FORCEINLINE int32 GetRefCount() const { return RefCount; }

    FORCEINLINE void AddRef() { ++RefCount; }

    FORCEINLINE FPropertyDesc* GetProperty(int32 Index) { return Index > INDEX_NONE && Index < Properties.Num() ? Properties[Index] : nullptr; }

    FORCEINLINE FFunctionDesc* GetFunction(int32 Index) { return Index > INDEX_NONE && Index < Functions.Num() ? Functions[Index] : nullptr; }

    bool Release(bool bKeepAlive = false);

    void Reset();

    template <typename CharType>
    FFieldDesc* RegisterField(const CharType *FieldName)
    {
        return RegisterField(FName(FieldName), this);
    }

    FFieldDesc* RegisterField(FName FieldName, FClassDesc *QueryClass);

    void GetInheritanceChain(TArray<FString> &NameChain, TArray<UStruct*> &StructChain) const;

    static EType GetType(UStruct *InStruct)
    {
        EType Type = EType::UNKNOWN;
        UScriptStruct *ScriptStruct = Cast<UScriptStruct>(InStruct);
        if (ScriptStruct)
        {
            Type = EType::SCRIPTSTRUCT;
        }
        else
        {
            UClass *Class = Cast<UClass>(InStruct);
            if (Class)
            {
                Type = EType::CLASS;
            }
        }
        return Type;
    }

private:
    union
    {
        UStruct *Struct;
        UScriptStruct *ScriptStruct;
        UClass *Class;
    };

    FString ClassName;
    FName ClassFName;
    TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ClassAnsiName;

    EType Type;
    uint8 UserdataPadding;            // only used for UScriptStruct
    int32 Size;
    int32 RefCount;

    FClassDesc *Parent;
    TArray<FClassDesc*> Interfaces;

    TMap<FName, FFieldDesc*> Fields;
    TArray<FPropertyDesc*> Properties;
    TArray<FFunctionDesc*> Functions;

    struct FFunctionCollection *FunctionCollection;
};

/**
 * Helper class to prevent releasing class descriptors
 */
class FScopedSafeClass
{
public:
    explicit FScopedSafeClass(FClassDesc *InClass)
    {
        AddClass(InClass);
    }

    explicit FScopedSafeClass(const TArray<FClassDesc*> &InClasses)
    {
        for (FClassDesc *InClass : InClasses)
        {
            AddClass(InClass);
        }
    }

    ~FScopedSafeClass()
    {
        for (FClassDesc *Class : Classes)
        {
            Class->Release(true);
        }
    }

private:
    void AddClass(FClassDesc *InClass)
    {
        if (InClass)
        {
            InClass->AddRef();
            Classes.Add(InClass);
        }
    }

    TArray<FClassDesc*> Classes;
};

/**
 * Field descriptor
 */
class FFieldDesc
{
    friend class FClassDesc;

public:
    FORCEINLINE bool IsValid() const { return FieldIndex != 0; }

    FORCEINLINE bool IsProperty() const { return FieldIndex > 0; }

    FORCEINLINE bool IsFunction() const { return FieldIndex < 0; }

    FORCEINLINE bool IsInherited() const { return OuterClass != QueryClass; }

    FORCEINLINE FPropertyDesc* AsProperty() const { return FieldIndex > 0 ? OuterClass->GetProperty(FieldIndex - 1) : nullptr; }

    FORCEINLINE FFunctionDesc* AsFunction() const { return FieldIndex < 0 ? OuterClass->GetFunction(-FieldIndex - 1) : nullptr; }

    FORCEINLINE FString GetOuterName() const { return OuterClass ? OuterClass->GetName() : TEXT(""); }

private:
    FFieldDesc()
        : QueryClass(nullptr), OuterClass(nullptr), FieldIndex(0)
    {}

    ~FFieldDesc() {}

    FClassDesc *QueryClass;
    FClassDesc *OuterClass;
    int32 FieldIndex;   // index in FClassDesc
};

/**
 * Enum descriptor
 */
class FEnumDesc
{
    enum class EType
    {
        Enum,
        UserDefinedEnum,
    };

public:
    explicit FEnumDesc(UEnum *InEnum);
    ~FEnumDesc() {}

    bool Release();

    FORCEINLINE bool IsValid() const { return Enum != nullptr; }

    FORCEINLINE const FString& GetName() const { return EnumName; }

    template <typename CharType>
    FORCEINLINE int64 GetValue(const CharType *EntryName) const
    {
        static int64 (*Func[2])(UEnum*, FName) = { FEnumDesc::GetEnumValue, FEnumDesc::GetUserDefinedEnumValue };
        return (Func[(int32)Type])(Enum, FName(EntryName));
    }

    FORCEINLINE UEnum* GetEnum() const { return Enum; }

    FORCEINLINE int32 GetRefCount() const { return RefCount; }

    FORCEINLINE void AddRef() { ++RefCount; }

    static int64 GetEnumValue(UEnum *Enum, FName EntryName);
    static int64 GetUserDefinedEnumValue(UEnum *Enum, FName EntryName);

private:
    union
    {
        UEnum *Enum;
        UUserDefinedEnum *UserDefinedEnum;
    };

    FString EnumName;
    int32 RefCount;
    EType Type;
};

/**
 * Reflection registry
 */
class FReflectionRegistry
{
public:
    ~FReflectionRegistry() { Cleanup(); }

    void Cleanup();

    template <typename CharType>
    FClassDesc* FindClass(const CharType *InName)
    {
        FClassDesc **ClassDesc = Name2Classes.Find(FName(InName));
        return ClassDesc ? *ClassDesc : nullptr;
    }

    bool UnRegisterClass(FClassDesc *ClassDesc);
    FClassDesc* RegisterClass(const TCHAR *InName, TArray<FClassDesc*> *OutChain = nullptr);
    FClassDesc* RegisterClass(UStruct *InStruct, TArray<FClassDesc*> *OutChain = nullptr);

    bool NotifyUObjectDeleted(const UObjectBase *InObject);

    template <typename CharType>
    FEnumDesc* FindEnum(const CharType *InName)
    {
        FEnumDesc **EnumDesc = Enums.Find(FName(InName));
        return EnumDesc ? *EnumDesc : nullptr;
    }

    template <typename CharType>
    bool UnRegisterEnumByName(const CharType *InName)
    {
        FName Name(InName);
        FEnumDesc **EnumDescPtr = Enums.Find(Name);
        if (EnumDescPtr)
        {
            FEnumDesc *EnumDesc = *EnumDescPtr;
            if (EnumDesc && EnumDesc->Release())
            {
                Enums.Remove(Name);
                return true;
            }
        }
        return false;
    }

    FEnumDesc* RegisterEnum(const TCHAR *InName);
    FEnumDesc* RegisterEnum(UEnum *InEnum);

    FFunctionDesc* RegisterFunction(UFunction *InFunction, int32 InFunctionRef = INDEX_NONE);
    bool UnRegisterFunction(UFunction *InFunction);
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    bool AddOverriddenFunction(UFunction *NewFunc, UFunction *OverriddenFunc);
    UFunction* RemoveOverriddenFunction(UFunction *NewFunc);
    UFunction* FindOverriddenFunction(UFunction *NewFunc);
#endif

#if UE_BUILD_DEBUG
    void Debug()
    {
        check(true);
    }
#endif

private:
    FClassDesc* RegisterClassInternal(const FString &ClassName, UStruct *Struct, FClassDesc::EType Type);
    void GetClassChain(FClassDesc *ClassDesc, TArray<FClassDesc*> *OutChain);

    TMap<FName, FClassDesc*> Name2Classes;
    TMap<UStruct*, FClassDesc*> Struct2Classes;
    TMap<UStruct*, FClassDesc*> NonNativeStruct2Classes;
    TMap<FName, FEnumDesc*> Enums;
    TMap<UFunction*, FFunctionDesc*> Functions;
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    TMap<UFunction*, UFunction*> OverriddenFunctions;
#endif
};

extern FReflectionRegistry GReflectionRegistry;

UNLUA_API int32 GetPropertyType(const UProperty *Property);
