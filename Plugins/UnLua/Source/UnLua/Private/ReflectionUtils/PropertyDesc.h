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

#include "lua.hpp"
#include "LowLevel.h"
#include "UnLuaBase.h"
#include "UnLuaCompatibility.h"
#include "UObject/WeakFieldPtr.h"

/**
 * new FProperty types
 */
enum
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
    CPT_MulticastSparseDelegate = CPT_Unused_Index_19,
#endif
    CPT_Enum = CPT_Unused_Index_21,
    CPT_Array = CPT_Unused_Index_22,
};

struct lua_State;

/**
 * Property descriptor
 */
class FPropertyDesc : public UnLua::ITypeInterface
{
public:
    static FPropertyDesc* Create(FProperty *InProperty);

    /**
     * Check the validity of this property
     *
     * @return - true if the property is valid, false otherwise
     */
    virtual bool IsValid() const override;

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
     * Test if this property is the reference parameter
     *
     * @return - true if the property is the return parameter, false otherwise
     */
    FORCEINLINE bool IsReferenceParameter() const { return Property->HasAnyPropertyFlags(CPF_ReferenceParm); }

    /**
     * Get the 'true' property
     *
     * @return - the FProperty
     */
    FORCEINLINE FProperty* GetProperty() const { return Property; }

    /**
     * @see FProperty::InitializeValue_InContainer(...)
     */
    FORCEINLINE void InitializeValue(void *ContainerPtr) const { Property->InitializeValue_InContainer(ContainerPtr); }

    /**
     * @see FProperty::DestroyValue_InContainer(...)
     */
    FORCEINLINE void DestroyValue(void *ContainerPtr) const { Property->DestroyValue_InContainer(ContainerPtr); }

    /**
     * @see FProperty::CopySingleValue(...)
     */
    FORCEINLINE void CopyValue(void *ContainerPtr, const void *Src) const { Property->CopySingleValue(Property->ContainerPtrToValuePtr<void>(ContainerPtr), Src); }

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

    virtual void Destruct(void* Dest) const override
    {
        if (PropertyPtr.IsValid())
            Property->DestroyValue(Dest);
    }

    virtual void Copy(void *Dest, const void *Src) const override { Property->CopySingleValue(Dest, Src); }
    virtual bool Identical(const void *A, const void *B) const override { return Property->Identical(A, B); }
    virtual FString GetName() const override { return Name; }
    virtual FProperty* GetUProperty() const override { return Property; }

    virtual void ReadValue_InContainer(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override 
    {
        if (UNLIKELY(!PropertyPtr.IsValid()))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to read invalid property '%s'"), *Name);
            lua_pushnil(L);
            return;
        }

        if (UnLua::LowLevel::IsReleasedPtr(ContainerPtr))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to read property '%s' on released object"), *Name);
            lua_pushnil(L);
            return;
        }

        GetValueInternal(L, PropertyPtr->ContainerPtrToValuePtr<void*>(ContainerPtr), bCreateCopy);
    }

    virtual void ReadValue(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override 
    {
        if (UNLIKELY(!PropertyPtr.IsValid()))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to read invalid property '%s'"), *Name);
            lua_pushnil(L);
            return;
        }

        if (UnLua::LowLevel::IsReleasedPtr(ValuePtr))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to read property '%s' on released object"), *Name);
            lua_pushnil(L);
            return;
        }
        
        GetValueInternal(L, ValuePtr, bCreateCopy);
    }

    virtual bool WriteValue_InContainer(lua_State *L, void *ContainerPtr, int32 IndexInStack, bool bCreateCopy) const override 
    {
        if (UNLIKELY(!PropertyPtr.IsValid()))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write invalid property %s"), *Name);
            lua_pushnil(L);
            return false;
        }

        if (UNLIKELY(UnLua::LowLevel::IsReleasedPtr(ContainerPtr)))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write property '%s' on released object"), *Name);
            return false;
        }

        return SetValueInternal(L, PropertyPtr->ContainerPtrToValuePtr<void*>(ContainerPtr), IndexInStack, bCreateCopy); 
    }

    virtual bool WriteValue(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCreateCopy) const override 
    {
        if (UNLIKELY(!PropertyPtr.IsValid()))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write invalid property %s"), *Name);
            lua_pushnil(L);
            return false;
        }

        if (UNLIKELY(UnLua::LowLevel::IsReleasedPtr(ValuePtr)))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write property '%s' on released object"), *Name);
            return false;
        }
        
        return SetValueInternal(L, ValuePtr, IndexInStack, bCreateCopy); 
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData = nullptr) { return true; };
#endif

    void SetPropertyType(int8 Type);
    int8 GetPropertyType();
	
protected:
    explicit FPropertyDesc(FProperty *InProperty);

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const = 0;

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack = -1, bool bCopyValue = true) const = 0;

    union
    {
        FProperty *Property;
        FNumericProperty *NumericProperty;
        FEnumProperty *EnumProperty;
        FBoolProperty *BoolProperty;
        FObjectPropertyBase *ObjectBaseProperty;
        FSoftObjectProperty *SoftObjectProperty;
        FInterfaceProperty *InterfaceProperty;
        FNameProperty *NameProperty;
        FStrProperty *StringProperty;
        FTextProperty *TextProperty;
        FArrayProperty *ArrayProperty;
        FMapProperty *MapProperty;
        FSetProperty *SetProperty;
        FStructProperty *StructProperty;
        FDelegateProperty *DelegateProperty;
        FMulticastDelegateProperty *MulticastDelegateProperty;
    };
    TWeakFieldPtr<FProperty> PropertyPtr;
    int8 PropertyType;
    FString Name = TEXT("");
};

UNLUA_API int32 GetPropertyType(const FProperty *Property);
