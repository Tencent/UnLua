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

#include "PropertyDesc.h"
#include "ClassDesc.h"
#include "ReflectionRegistry.h"
#include "LuaCore.h"
#include "LuaContext.h"
#include "DelegateHelper.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"
#include "UEObjectReferencer.h"

TMap<FProperty*,FPropertyDesc*> FPropertyDesc::Property2Desc;

FPropertyDesc::FPropertyDesc(FProperty *InProperty) : Property(InProperty) 
{ 
	GReflectionRegistry.AddToDescSet(this, DESC_PROPERTY); 
    Property2Desc.Add(Property,this);
    PropertyType = CPT_None;
}

FPropertyDesc::~FPropertyDesc()
{
	GReflectionRegistry.RemoveFromDescSet(this);
    Property2Desc.Remove(Property);
}

bool FPropertyDesc::IsValid() const
{
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 25
    return GLuaCxt->IsUObjectValid(Property);
#else
    bool bValid = false;
    if (Property)
    {
        bValid = true;

        switch (PropertyType)
        {
            case CPT_Interface:
            {
                bValid = GLuaCxt->IsUObjectValid(((FInterfaceProperty*)Property)->InterfaceClass);
                break;
            }
            case CPT_Delegate:
            {
                bValid = GLuaCxt->IsUObjectValid(((FDelegateProperty*)Property)->SignatureFunction);
                break;
            }
            case CPT_MulticastDelegate:
            case CPT_MulticastSparseDelegate:
            {
                bValid = GLuaCxt->IsUObjectValid(((FMulticastDelegateProperty*)Property)->SignatureFunction);
                break;
            }
            case CPT_Struct:
            {
                bValid = GLuaCxt->IsUObjectValid(((FStructProperty*)Property)->Struct);
                break;
            }
            case CPT_ObjectReference:
            case CPT_WeakObjectReference:
            case CPT_LazyObjectReference:
            case CPT_SoftObjectReference:
            {
                bValid = GLuaCxt->IsUObjectValid(((FObjectPropertyBase*)Property)->PropertyClass);
                break;
            }
        }
    }

    return bValid;
#endif
}

/**
 * Integer property descriptor
 */
class FIntegerPropertyDesc : public FPropertyDesc
{
public:
    explicit FIntegerPropertyDesc(FProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushIntegerArray(L, NumericProperty, (void*)ValuePtr);
        }
        else
        {
            lua_pushinteger(L, NumericProperty->GetUnsignedIntPropertyValue(ValuePtr));
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        NumericProperty->SetIntPropertyValue(ValuePtr, (uint64)lua_tointeger(L, IndexInStack));
        return false;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("integer needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
            else
            {
                if (!lua_isinteger(L, IndexInStack))
                {
                    ErrorMsg = FString::Printf(TEXT("integer needed but got float or double"));
                    return false;
                }
            }
        }

        return true;
    };
#endif
};

/**
 * Float property descriptor
 */
class FFloatPropertyDesc : public FPropertyDesc
{
public:
    explicit FFloatPropertyDesc(FProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushFloatArray(L, NumericProperty, (void*)ValuePtr);
        }
        else
        {
            lua_pushnumber(L, NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        NumericProperty->SetFloatingPointPropertyValue(ValuePtr, lua_tonumber(L, IndexInStack));
        return false;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("number needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * Enum property descriptor
 */
class FEnumPropertyDesc : public FPropertyDesc
{
public:
    explicit FEnumPropertyDesc(FProperty *InProperty)
        : FPropertyDesc(InProperty)
    {
        RegisterEnum(*GLuaCxt, EnumProperty->GetEnum());
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushEnumArray(L, EnumProperty->GetUnderlyingProperty(), (void*)ValuePtr);
        }
        else
        {
            lua_pushinteger(L, EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, lua_tointeger(L, IndexInStack));
        return false;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("integer needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
            else
            {
                if (!lua_isinteger(L, IndexInStack))
                {
                    ErrorMsg = FString::Printf(TEXT("integer needed but got float or double"));
                    return false;
                }
            }
        }

        return true;
    };
#endif
};

/**
 * Bool property descriptor
 */
class FBoolPropertyDesc : public FPropertyDesc
{
public:
    explicit FBoolPropertyDesc(FProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        lua_pushboolean(L, BoolProperty->GetPropertyValue(ValuePtr));
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {  
        BoolProperty->SetPropertyValue(ValuePtr, lua_toboolean(L, IndexInStack) != 0);
        return false;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TBOOLEAN)
            {
                ErrorMsg = FString::Printf(TEXT("bool needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }
        return true;
    };
#endif
};

/**
 * UObject property descriptor
 */
class FObjectPropertyDesc : public FPropertyDesc
{
public:
    explicit FObjectPropertyDesc(FProperty *InProperty, bool bSoftObject)
        : FPropertyDesc(InProperty), MetaClass(nullptr), IsSoftObject(bSoftObject)
    {
        if (ObjectBaseProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
        {
            MetaClass = bSoftObject ? (((FSoftClassProperty*)Property)->MetaClass) : ((FClassProperty*)Property)->MetaClass;
        }
        RegisterClass(*GLuaCxt, MetaClass ? MetaClass : ObjectBaseProperty->PropertyClass);     // register meta/property class first
    }

    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) override
    {
        UObject *Object = UnLua::GetUObject(L, SrcIndexInStack);
        if (Object && DestContainerPtr && IsOutParameter())
        {
            if (MetaClass)
            {
                UClass *Class = Cast<UClass>(Object);
                if (Class && !Class->IsChildOf(MetaClass))
                {
                    Object = nullptr;
                }
            }
            ObjectBaseProperty->SetObjectPropertyValue(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Object);
            return true;
        }
        return false;
    }

    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) override
    {
        void **Userdata = (void**)GetUserdata(L, DestIndexInStack);
        if (Userdata && IsOutParameter())
        {
            *Userdata = ObjectBaseProperty->GetObjectPropertyValue(Property->ContainerPtrToValuePtr<void>(SrcContainerPtr));
            return true;
        }
        return false;
    }

    virtual bool CopyBack(void *Dest, const void *Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            ObjectBaseProperty->SetObjectPropertyValue(Dest, *((UObject**)Src));
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushObjectArray(L, ObjectBaseProperty, (void*)ValuePtr);
        }
        else
        {
            if (IsSoftObject)
            {
                UnLua::PushUObject(L, SoftObjectProperty->GetObjectPropertyValue(ValuePtr));
            }
            else
            {
                UnLua::PushUObject(L, ObjectBaseProperty->GetObjectPropertyValue(ValuePtr));
            }
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        if (MetaClass)
        {
            UClass *Value = Cast<UClass>(UnLua::GetUObject(L, IndexInStack));
            if (Value && !Value->IsChildOf(MetaClass))
            {
                Value = nullptr;
            }
            ObjectBaseProperty->SetObjectPropertyValue(ValuePtr, Value);
        }
        else
        {
            UObject* Object = UnLua::GetUObject(L, IndexInStack);
#if ENABLE_TYPE_CHECK == 1
            if (Object)
            {
                if (!Object->GetClass()->IsChildOf(ObjectBaseProperty->PropertyClass))
                {
                    UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("Invalid value type : property.type=%s, value.type=%s"), *ObjectBaseProperty->PropertyClass->GetName(), *Object->GetClass()->GetName());
                }
            }
#endif
            ObjectBaseProperty->SetObjectPropertyValue(ValuePtr, Object);
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;
        UObject* Object = UnLua::GetUObject(L, IndexInStack);
        if (Object)
        {
            if (MetaClass)
            {
                UClass* Class = Cast<UClass>(Object);
                if (Class && !Class->IsChildOf(MetaClass))
                {
                    ErrorMsg = FString::Printf(TEXT("class %s needed but got %s"), *MetaClass->GetName(), *Class->GetName());
                    return false;
                }
            }
            else
            {
                UClass* Class = Object->GetClass();
                if (Class && !Class->IsChildOf(ObjectBaseProperty->PropertyClass))
                {
                    ErrorMsg = FString::Printf(TEXT("object %s needed but got %s"), *ObjectBaseProperty->PropertyClass->GetName(), *Class->GetName());
                    return false;
                }
            }
        }

        return true;
    };
#endif

private:
    UClass *MetaClass;
    bool IsSoftObject;
};

/**
 * FSoftObject property descriptor
 */
class FSoftObjectPropertyDesc : public FPropertyDesc
{
public:
    explicit FSoftObjectPropertyDesc(FProperty* InProperty)
        : FPropertyDesc(InProperty)
    {
    }

    ~FSoftObjectPropertyDesc()
    {
        // release element property descriptor
    }

    virtual bool CopyBack(lua_State* L, int32 SrcIndexInStack, void* DestContainerPtr) override
    {
        void* Value = GetCppInstanceFast(L, SrcIndexInStack);
        return Value ? CopyBack(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Value) : false;
    }

    virtual bool CopyBack(lua_State* L, void* SrcContainerPtr, int32 DestIndexInStack) override
    {
        void* Value = GetCppInstanceFast(L, DestIndexInStack);
        return Value ? CopyBack(Value, Property->ContainerPtrToValuePtr<void>(SrcContainerPtr)) : false;
    }

    virtual bool CopyBack(void* Dest, const void* Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            FMemory::Memcpy(Dest, Src, Property->GetSize());        // shallow copy is enough
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State* L, const void* ValuePtr, bool bCreateCopy) const override
    {
        if (bCreateCopy)
        {
            void* Userdata = NewUserdataWithPadding(L, Property->GetSize(), "FSoftObjectPtr", 0);
            Property->InitializeValue(Userdata);
            Property->CopySingleValue(Userdata, ValuePtr);
        }
        else
        {
            if (Property->ArrayDim > 1)
            {
                PushStructArray(L, Property, (void*)ValuePtr, "FSoftObjectPtr");
            }
            else
            {
                UnLua::PushPointer(L, (void*)ValuePtr, "FSoftObjectPtr", false);
            }
        }
    }

    virtual bool SetValueInternal(lua_State* L, void* ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        void* Value = GetCppInstanceFast(L, IndexInStack);
        if (Value)
        {
            if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
            {
                FMemory::Memcpy(ValuePtr, Value, Property->GetSize());           // shallow copy
                return false;
            }
            else
            {
                Property->CopySingleValue(ValuePtr, Value);
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;

        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TUSERDATA)
            {
                ErrorMsg = FString::Printf(TEXT("userdata is needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }

            if (1 != lua_getmetatable(L, IndexInStack))
            {
                ErrorMsg = FString::Printf(TEXT("metatable of userdata is needed but got nil"));
                return false;
            }

            lua_pushstring(L, "__name");
            lua_rawget(L, -2);
            FString MetatableName = lua_tostring(L, -1);
            if (MetatableName.IsEmpty())
            {
                ErrorMsg = FString::Printf(TEXT("metatable name of userdata needed but got nil"));
                return false;
            }

            if (!MetatableName.Equals("FSoftObjectPtr"))
            {
                ErrorMsg = FString::Printf(TEXT("metatable name of userdata FSoftObjectPtr needed but got %s"), *MetatableName);
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * Interface property descriptor
 */
class FInterfacePropertyDesc : public FPropertyDesc
{
public:
    explicit FInterfacePropertyDesc(FProperty *InProperty)
        : FPropertyDesc(InProperty)
    {
        RegisterClass(*GLuaCxt, InterfaceProperty->InterfaceClass);         // register interface class first
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushInterfaceArray(L, InterfaceProperty, (void*)ValuePtr);
        }
        else
        {
            const FScriptInterface &Interface = InterfaceProperty->GetPropertyValue(ValuePtr);
            UnLua::PushUObject(L, Interface.GetObject());
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        FScriptInterface *Interface = (FScriptInterface*)ValuePtr;
        UObject *Value = UnLua::GetUObject(L, IndexInStack);
        Interface->SetObject(Value);
        Interface->SetInterface(Value ? Value->GetInterfaceAddress(InterfaceProperty->InterfaceClass) : nullptr);
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;

        UObject* Object = UnLua::GetUObject(L, IndexInStack);
        if (Object)
        {
            UClass* Class = Object->GetClass();
            if ((Class)
                && (!Class->ImplementsInterface(InterfaceProperty->InterfaceClass)))
            {
                ErrorMsg = FString::Printf(TEXT("implements of interface %s is needed but got nil for object %s"), *InterfaceProperty->InterfaceClass->GetName(), *Class->GetName());
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * Name property descriptor
 */
class FNamePropertyDesc : public FPropertyDesc
{
public:
    explicit FNamePropertyDesc(FProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushFNameArray(L, NameProperty, (void*)ValuePtr);
        }
        else
        {
            lua_pushstring(L, TCHAR_TO_UTF8(*NameProperty->GetPropertyValue(ValuePtr).ToString()));
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        NameProperty->SetPropertyValue(ValuePtr, FName(lua_tostring(L, IndexInStack)));
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TSTRING && Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("string needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * String property descriptor
 */
class FStringPropertyDesc : public FPropertyDesc
{
public:
    explicit FStringPropertyDesc(FProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushFStringArray(L, StringProperty, (void*)ValuePtr);
        }
        else
        {
            lua_pushstring(L, TCHAR_TO_UTF8(*StringProperty->GetPropertyValue(ValuePtr)));
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        StringProperty->SetPropertyValue(ValuePtr, UTF8_TO_TCHAR(lua_tostring(L, IndexInStack)));
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TSTRING && Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("string needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * Text property descriptor
 */
class FTextPropertyDesc : public FPropertyDesc
{
public:
    explicit FTextPropertyDesc(FProperty* InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State* L, const void* ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushFTextArray(L, TextProperty, (void*)ValuePtr);
        }
        else
        {
            lua_pushstring(L, TCHAR_TO_UTF8(*TextProperty->GetPropertyValue(ValuePtr).ToString()));
        }
    }

    virtual bool SetValueInternal(lua_State* L, void* ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        TextProperty->SetPropertyValue(ValuePtr, FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, IndexInStack))));
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TSTRING && Type != LUA_TNUMBER)
            {
                ErrorMsg = FString::Printf(TEXT("string needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * TArray property descriptor
 */
class FArrayPropertyDesc : public FPropertyDesc, public TLuaContainerInterface<FLuaArray>
{
public:
    explicit FArrayPropertyDesc(FProperty *InProperty)
        : FPropertyDesc(InProperty), InnerProperty(FPropertyDesc::Create(ArrayProperty->Inner))
    {}

    ~FArrayPropertyDesc()
    {
        InnerProperty.Reset();                                              // release inner property descriptor
    }

    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) override
    {
        FScriptArray *Src = (FScriptArray*)GetScriptContainer(L, SrcIndexInStack);
        return Src ? CopyBack(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Src) : false;
    }

    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) override
    {
        FScriptArray *Dest = (FScriptArray*)GetScriptContainer(L, DestIndexInStack);
        return Dest ? CopyBack(Dest, Property->ContainerPtrToValuePtr<void>(SrcContainerPtr)) : false;
    }

    virtual bool CopyBack(void *Dest, const void *Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            FMemory::Memcpy(Dest, Src, sizeof(FScriptArray));        // shallow copy is enough
            //ArrayProperty->CopyCompleteValue(Dest, Src);
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (bCreateCopy)
        {
            FScriptArray *ScriptArray = new FScriptArray;
            //ArrayProperty->InitializeValue(ScriptArray);
            ArrayProperty->CopyCompleteValue(ScriptArray, ValuePtr);
            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Array);
            new(Userdata) FLuaArray(ScriptArray, InnerProperty, FLuaArray::OwnedBySelf);
        }
        else
        {
            FScriptArray *ScriptArray = (FScriptArray*)(&ArrayProperty->GetPropertyValue(ValuePtr));
            void *Userdata = CacheScriptContainer(L, ScriptArray, FScriptContainerDesc::Array);
            if (Userdata)
            {
                FLuaArray *LuaArray = new(Userdata) FLuaArray(ScriptArray, (TLuaContainerInterface<FLuaArray>*)this, FLuaArray::OwnedByOther);
                ((TLuaContainerInterface<FLuaArray>*)this)->AddContainer(LuaArray);
            }
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type == LUA_TTABLE)
        {
            FScriptArray ScriptArray;
            FLuaArray LuaArray(&ScriptArray, InnerProperty, FLuaArray::OwnedByOther);
            TraverseTable(L, IndexInStack, &LuaArray, FArrayPropertyDesc::FillArray);       // fill table elements
            ArrayProperty->CopyCompleteValue(ValuePtr, &ScriptArray);
        }
        else if (Type == LUA_TUSERDATA)
        {
            FScriptArray *Src = (FScriptArray*)GetScriptContainer(L, IndexInStack);
            if (Src)
            {
                if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
                {
                    FMemory::Memcpy(ValuePtr, Src, sizeof(FScriptArray));                   // shallow copy
                    return false;
                }
                else
                {
                    ArrayProperty->CopyCompleteValue(ValuePtr, Src);
                    //ArrayProperty->SetPropertyValue(ValuePtr, *LuaArray->ScriptArray);    // copy constructor of FScriptArray doesn't work
                }
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;

        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if ((Type != LUA_TTABLE)
                && (Type != LUA_TUSERDATA))
            {
                ErrorMsg = FString::Printf(TEXT("table or userdata needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }

            if (Type == LUA_TUSERDATA)
            {
                int32 RetValue = lua_getmetatable(L, IndexInStack);
                if (RetValue != 1)
                {
                    ErrorMsg = FString::Printf(TEXT("metatable of userdata needed but got nil"));
                    return false;
                }

                lua_pushstring(L, "__name");
                lua_rawget(L, -2);
                FString MetatableName = lua_tostring(L, -1);
                if (MetatableName.IsEmpty() || !MetatableName.Equals("TArray"))
                {
                    ErrorMsg = FString::Printf(TEXT("metatable name of userdata TArray needed but got %s"), *MetatableName);
                    return false;
                }
            }
        }

        return true;
    };
#endif

    // interfaces from 'TLuaContainerInterface<FLuaArray>'
    virtual TSharedPtr<UnLua::ITypeInterface> GetInnerInterface() const override { return InnerProperty; }
    virtual TSharedPtr<UnLua::ITypeInterface> GetExtraInterface() const override { return TSharedPtr<UnLua::ITypeInterface>(); }
    static bool FillArray(lua_State *L, void *Userdata)
    {
        FLuaArray *Array = (FLuaArray*)Userdata;
        int32 Index = Array->AddDefaulted();
        uint8 *Data = Array->GetData(Index);
        Array->Inner->Write(L, Data, -1);
        return true;
    }

private:
    TSharedPtr<UnLua::ITypeInterface> InnerProperty;
};


/**
 * TMap property descriptor
 */
class FMapPropertyDesc : public FPropertyDesc, public TLuaContainerInterface<FLuaMap>
{
public:
    explicit FMapPropertyDesc(FProperty *InProperty)
        : FPropertyDesc(InProperty), KeyProperty(FPropertyDesc::Create(MapProperty->KeyProp)), ValueProperty(FPropertyDesc::Create(MapProperty->ValueProp))
    {}

    ~FMapPropertyDesc()
    {
        KeyProperty.Reset();                // release key property descriptor
        ValueProperty.Reset();              // release value property descriptor
    }

    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) override
    {
        FScriptMap *Src = (FScriptMap*)GetScriptContainer(L, SrcIndexInStack);
        return Src ? CopyBack(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Src) : false;
    }

    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) override
    {
        FScriptMap *Dest = (FScriptMap*)GetScriptContainer(L, DestIndexInStack);
        return Dest ? CopyBack(Dest, Property->ContainerPtrToValuePtr<void>(SrcContainerPtr)) : false;
    }

    virtual bool CopyBack(void *Dest, const void *Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            FMemory::Memcpy(Dest, Src, sizeof(FScriptMap));        // shallow copy is enough
            //MapProperty->CopyCompleteValue(Dest, Src);
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (bCreateCopy)
        {
            FScriptMap *ScriptMap = new FScriptMap;
            //MapProperty->InitializeValue(ScriptMap);
            MapProperty->CopyCompleteValue(ScriptMap, ValuePtr);
            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Map);
            new(Userdata) FLuaMap(ScriptMap, KeyProperty, ValueProperty, FLuaMap::OwnedBySelf);
        }
        else
        {
            FScriptMap *ScriptMap = (FScriptMap*)(&MapProperty->GetPropertyValue(ValuePtr));
            void *Userdata = CacheScriptContainer(L, ScriptMap, FScriptContainerDesc::Map);
            if (Userdata)
            {
                FLuaMap *LuaMap = new(Userdata) FLuaMap(ScriptMap, (TLuaContainerInterface<FLuaMap>*)this, FLuaMap::OwnedByOther);
                ((TLuaContainerInterface<FLuaMap>*)this)->AddContainer(LuaMap);
            }
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type == LUA_TTABLE)
        {
            FScriptMap ScriptMap;
            FLuaMap LuaMap(&ScriptMap, KeyProperty, ValueProperty, FLuaMap::OwnedByOther);
            TraverseTable(L, IndexInStack, &LuaMap, FMapPropertyDesc::FillMap);
            ArrayProperty->CopyCompleteValue(ValuePtr, &ScriptMap);
        }
        else if (Type == LUA_TUSERDATA)
        {
            FLuaMap *LuaMap = (FLuaMap*)GetCppInstanceFast(L, IndexInStack);
            if (LuaMap)
            {
                if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
                {
                    FMemory::Memcpy(ValuePtr, LuaMap->Map, sizeof(FScriptMap));     // shallow copy
                    return false;
                }
                else
                {
                    MapProperty->CopyCompleteValue(ValuePtr, LuaMap->Map);
                }
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;

        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if ((Type != LUA_TTABLE)
                && (Type != LUA_TUSERDATA))
            {
                ErrorMsg = FString::Printf(TEXT("table or userdata needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }

            if (Type == LUA_TUSERDATA)
            {
                int32 RetValue = lua_getmetatable(L, IndexInStack);
                if (RetValue != 1)
                {
                    ErrorMsg = FString::Printf(TEXT("metatable of userdata needed but got nil"));
                    return false;
                }

                lua_pushstring(L, "__name");
                lua_rawget(L, -2);
                FString MetatableName = lua_tostring(L, -1);
                if (MetatableName.IsEmpty() || !MetatableName.Equals("TMap"))
                {
                    ErrorMsg = FString::Printf(TEXT("metatable name of userdata TMap needed but got %s"), *MetatableName);
                    return false;
                }
            }
        }

        return true;
    };
#endif

    // interfaces from 'TLuaContainerInterface<FLuaMap>'
    virtual TSharedPtr<UnLua::ITypeInterface> GetInnerInterface() const override { return KeyProperty; }
    virtual TSharedPtr<UnLua::ITypeInterface> GetExtraInterface() const override { return ValueProperty; }
    static bool FillMap(lua_State *L, void *Userdata)
    {
        FLuaMap *Map = (FLuaMap*)Userdata;
        void *ValueCache = (uint8*)Map->ElementCache + Map->MapLayout.ValueOffset;
        Map->KeyInterface->Initialize(Map->ElementCache);
        Map->ValueInterface->Initialize(ValueCache);
        Map->KeyInterface->Write(L, Map->ElementCache, -2);
        Map->ValueInterface->Write(L, Map->ValueInterface->GetOffset() > 0 ? Map->ElementCache : ValueCache, -1);
        Map->Add(Map->ElementCache, ValueCache);
        return true;
    }

private:
    TSharedPtr<UnLua::ITypeInterface> KeyProperty;
    TSharedPtr<UnLua::ITypeInterface> ValueProperty;
};

/**
 * TSet property descriptor
 */
class FSetPropertyDesc : public FPropertyDesc, public TLuaContainerInterface<FLuaSet>
{
public:
    explicit FSetPropertyDesc(FProperty *InProperty)
        : FPropertyDesc(InProperty), InnerProperty(FPropertyDesc::Create(SetProperty->ElementProp))
    {}

    ~FSetPropertyDesc()
    {
        InnerProperty.Reset();                                                  // release element property descriptor
    }

    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) override
    {
        FScriptSet *Src = (FScriptSet*)GetScriptContainer(L, SrcIndexInStack);
        return Src ? CopyBack(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Src) : false;
    }

    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) override
    {
        FScriptSet *Dest = (FScriptSet*)GetScriptContainer(L, DestIndexInStack);
        return Dest ? CopyBack(Dest, Property->ContainerPtrToValuePtr<void>(SrcContainerPtr)) : false;
    }

    virtual bool CopyBack(void *Dest, const void *Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            FMemory::Memcpy(Dest, Src, sizeof(FScriptSet));        // shallow copy is enough
            //SetProperty->CopyCompleteValue(Dest, Src);
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (bCreateCopy)
        {
            FScriptSet *ScriptSet = new FScriptSet;
            //SetProperty->InitializeValue(ScriptSet);
            SetProperty->CopyCompleteValue(ScriptSet, ValuePtr);
            void *Userdata = NewScriptContainer(L, FScriptContainerDesc::Set);
            new(Userdata) FLuaSet(ScriptSet, InnerProperty, FLuaSet::OwnedBySelf);
        }
        else
        {
            FScriptSet *ScriptSet = (FScriptSet*)(&SetProperty->GetPropertyValue(ValuePtr));
            void *Userdata = CacheScriptContainer(L, ScriptSet, FScriptContainerDesc::Set);
            if (Userdata)
            {
                FLuaSet *LuaSet = new(Userdata) FLuaSet(ScriptSet, (TLuaContainerInterface<FLuaSet>*)this, FLuaSet::OwnedByOther);
                ((TLuaContainerInterface<FLuaSet>*)this)->AddContainer(LuaSet);
            }
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type == LUA_TTABLE)
        {
            FScriptSet ScriptSet;
            FLuaSet LuaSet(&ScriptSet, InnerProperty, FLuaSet::OwnedByOther);
            TraverseTable(L, IndexInStack, &LuaSet, FSetPropertyDesc::FillSet);
            ArrayProperty->CopyCompleteValue(ValuePtr, &ScriptSet);
        }
        else if (Type == LUA_TUSERDATA)
        {
            FLuaSet *LuaSet = (FLuaSet*)GetCppInstanceFast(L, IndexInStack);
            if (LuaSet)
            {
                if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
                {
                    FMemory::Memcpy(ValuePtr, LuaSet->Set, sizeof(FScriptSet));     // shallow copy
                    return false;
                }
                else
                {
                    SetProperty->CopyCompleteValue(ValuePtr, LuaSet->Set);
                }
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;

        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if ((Type != LUA_TTABLE)
                && (Type != LUA_TUSERDATA))
            {
                ErrorMsg = FString::Printf(TEXT("table or userdata needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }

            if (Type == LUA_TUSERDATA)
            {
                int32 RetValue = lua_getmetatable(L, IndexInStack);
                if (RetValue != 1)
                {
                    ErrorMsg = FString::Printf(TEXT("metatable of userdata needed but got nil"));
                    return false;
                }

                lua_pushstring(L, "__name");
                lua_rawget(L, -2);
                FString MetatableName = lua_tostring(L, -1);
                if (MetatableName.IsEmpty() || !MetatableName.Equals("TSet"))
                {
                    ErrorMsg = FString::Printf(TEXT("metatable name of userdata TSet needed but got %s"), *MetatableName);
                    return false;
                }
            }
        }

        return true;
    };
#endif

    // interfaces from 'TLuaContainerInterface<FLuaSet>'
    virtual TSharedPtr<UnLua::ITypeInterface> GetInnerInterface() const override { return InnerProperty; }
    virtual TSharedPtr<UnLua::ITypeInterface> GetExtraInterface() const override { return TSharedPtr<UnLua::ITypeInterface>(); }
    static bool FillSet(lua_State *L, void *Userdata)
    {
        FLuaSet *Set = (FLuaSet*)Userdata;
        Set->ElementInterface->Initialize(Set->ElementCache);
        Set->ElementInterface->Write(L, Set->ElementCache, -1);
        Set->Add(Set->ElementCache);
        return true;
    }

private:
    TSharedPtr<UnLua::ITypeInterface> InnerProperty;
};

void FPropertyDesc::SetPropertyType(int8 Type)
{
    PropertyType = Type;
}

int8 FPropertyDesc::GetPropertyType()
{
    return PropertyType;
}

class FStructPropertyDesc : public FPropertyDesc
{
public:
	explicit FStructPropertyDesc(FProperty* InProperty)
		: FPropertyDesc(InProperty), bFirstPropOfScriptStruct(GetPropertyOuter(Property)->IsA<UScriptStruct>() && Property->GetOffset_ForInternal() == 0)
	{}

protected:
	bool bFirstPropOfScriptStruct;
};


/**
 * ScriptStruct property descriptor
 */
class FScriptStructPropertyDesc : public FStructPropertyDesc
{
public:
    explicit FScriptStructPropertyDesc(FProperty *InProperty)
        : FStructPropertyDesc(InProperty), StructName(*FString::Printf(TEXT("F%s"), *StructProperty->Struct->GetName()))
    {
        FClassDesc *ClassDesc = RegisterClass(*GLuaCxt, StructProperty->Struct);    // register UScriptStruct first
        StructSize = ClassDesc->GetSize();
        UserdataPadding = ClassDesc->GetUserdataPadding();                          // padding size for userdata

        //ClassDesc->AddRef();
    }

    FScriptStructPropertyDesc(FProperty *InProperty, bool bDynamicallyCreated)
        : FStructPropertyDesc(InProperty), StructName(*FString::Printf(TEXT("F%s"), *StructProperty->Struct->GetName()))
    {
        FClassDesc *ClassDesc = RegisterClass(*GLuaCxt, StructProperty->Struct);
        StructSize = ClassDesc->GetSize();
        UserdataPadding = ClassDesc->GetUserdataPadding();
        bFirstPropOfScriptStruct = false;
    }

    ~FScriptStructPropertyDesc()
    {
        /*FClassDesc* ClassDesc = GReflectionRegistry.FindClass(StructName.Get());
        if (ClassDesc)
        {
            ClassDesc->SubRef();
            if (ClassDesc->GetRefCount() <= 0)
            {   
                // give a chance to release scriptstruct
                UScriptStruct* ScriptStruct = StructProperty->Struct;
                if (!ScriptStruct->IsNative())
                {
                    GObjectReferencer.RemoveObjectRef(ScriptStruct);
                }
                GReflectionRegistry.UnRegisterClass(ClassDesc);
            }
        }*/
    }

    virtual bool CopyBack(lua_State *L, int32 SrcIndexInStack, void *DestContainerPtr) override
    {
        void *Src = GetCppInstanceFast(L, SrcIndexInStack);
        return CopyBack(Property->ContainerPtrToValuePtr<void>(DestContainerPtr), Src);
    }

    virtual bool CopyBack(lua_State *L, void *SrcContainerPtr, int32 DestIndexInStack) override
    {
        void *Dest = GetCppInstanceFast(L, DestIndexInStack);
        return CopyBack(Dest, Property->ContainerPtrToValuePtr<void>(SrcContainerPtr));
    }

    virtual bool CopyBack(void *Dest, const void *Src) override
    {
        if (Dest && Src && IsOutParameter())
        {
            FMemory::Memcpy(Dest, Src, StructSize);        // shallow copy is enough
            //StructProperty->CopySingleValue(Dest, Src);
            return true;
        }
        return false;
    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (bCreateCopy)
        {
            void *Userdata = NewUserdataWithPadding(L, StructSize, StructName.Get(), UserdataPadding);
            StructProperty->InitializeValue(Userdata);
            StructProperty->CopySingleValue(Userdata, ValuePtr);
        }
        else
        {
            if (Property->ArrayDim > 1)
            {
                PushStructArray(L, Property, (void*)ValuePtr, StructName.Get());
            }
            else
            {
                UnLua::PushPointer(L, (void*)ValuePtr, StructName.Get(), bFirstPropOfScriptStruct);
            }
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        void *Value = GetCppInstanceFast(L, IndexInStack);
        if (Value)
        {
            if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
            {
                FMemory::Memcpy(ValuePtr, Value, StructSize);           // shallow copy
                return false;
            }
            else
            {
                StructProperty->CopySingleValue(ValuePtr, Value);
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack;
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TUSERDATA)
            {
                ErrorMsg = FString::Printf(TEXT("userdata needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
            int32 RetValue = lua_getmetatable(L, IndexInStack);
            if (RetValue != 1)
            {
                ErrorMsg = FString::Printf(TEXT("metatable of userdata needed but got nil"));
                return false;
            }

            lua_pushstring(L, "__name");
            lua_rawget(L, -2);
            const char* MetatableName = lua_tostring(L, -1);
            if (!MetatableName)
            {
                ErrorMsg = FString::Printf(TEXT("metatable name of userdata needed but got nil"));
                return false;
            }

            FClassDesc* CurrentClassDesc = GReflectionRegistry.FindClass(MetatableName);
            if (!CurrentClassDesc)
            {
                ErrorMsg = FString::Printf(TEXT("metatable of userdata needed in registry but got no found"));
                return false;
            }

            UScriptStruct* ScriptStruct = CurrentClassDesc->AsScriptStruct();
            if (!ScriptStruct || !ScriptStruct->IsChildOf(StructProperty->Struct))
            {
                ErrorMsg = FString::Printf(TEXT("struct %s needed but got %s"), *StructProperty->Struct->GetName(), *ScriptStruct->GetName());
                return false;
            }
        }

        return true;
    };
#endif

private:
    TStringConversion<TStringConvert<TCHAR, ANSICHAR>> StructName;
    int32 StructSize;
    uint8 UserdataPadding;
};

/**
 * Delegate property descriptor
 */
class FDelegatePropertyDesc : public FStructPropertyDesc
{
public:
    explicit FDelegatePropertyDesc(FProperty *InProperty) : FStructPropertyDesc(InProperty) {}

    virtual void Read(lua_State* L, const void* ContainerPtr, bool bCreateCopy) const override
    {
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        FScriptDelegate* ScriptDelegate = (FScriptDelegate*)DelegateProperty->GetPropertyValuePtr(ValuePtr);
		GetValueInternal(L, ScriptDelegate, bCreateCopy);

    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {   
            PushDelegateArray(L, DelegateProperty, (void*)ValuePtr);
        }
        else
        {
            FScriptDelegate *ScriptDelegate = DelegateProperty->GetPropertyValuePtr((void*)ValuePtr);
            FDelegateHelper::PreBind(ScriptDelegate, DelegateProperty);
            UnLua::PushPointer(L, ScriptDelegate, "FScriptDelegate", bFirstPropOfScriptStruct);
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        int32 FuncIdxInTable = GetDelegateInfo(L, IndexInStack, Object, CallbackFunction);      // get target UObject and Lua function
        if (FuncIdxInTable != INDEX_NONE)
        {
            FScriptDelegate *ScriptDelegate = DelegateProperty->GetPropertyValuePtr(ValuePtr);
            FCallbackDesc Callback(Object->GetClass(), CallbackFunction, Object);
            FName FuncName = FDelegateHelper::GetBindedFunctionName(Callback);
            if (FuncName == NAME_None)
            {
                // no delegate function is created yet
                lua_rawgeti(L, IndexInStack, FuncIdxInTable);
                int32 CallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
                FDelegateHelper::Bind(ScriptDelegate, DelegateProperty, Object, Callback, CallbackRef);
            }
            else
            {
				UE_LOG(UnLuaDelegate, Verbose, TEXT("[BindUFunction FuncName:%s %p]"), *FuncName.ToString(), Callback.CallbackFunction);
                ScriptDelegate->BindUFunction(Object, FuncName);        // a delegate function is created already
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TTABLE)
            {
                ErrorMsg = FString::Printf(TEXT("table needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};

/**
 * Multicast delegate property descriptor
 */
template <typename T>
class TMulticastDelegatePropertyDesc : public FStructPropertyDesc
{
public:
    explicit TMulticastDelegatePropertyDesc(FProperty *InProperty) : FStructPropertyDesc(InProperty) {}

    virtual void Read(lua_State* L, const void* ContainerPtr, bool bCreateCopy) const override
    {
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        T* ScriptDelegate = (T*)ValuePtr;
		GetValueInternal(L, ScriptDelegate, bCreateCopy);

    }

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        if (Property->ArrayDim > 1)
        {
            PushMCDelegateArray(L, MulticastDelegateProperty, (void*)ValuePtr, TMulticastDelegateTraits<T>::GetName());
        }
        else
        {
            T *ScriptDelegate = (T*)ValuePtr;
            FDelegateHelper::PreAdd(ScriptDelegate, MulticastDelegateProperty);
            UnLua::PushPointer(L, ScriptDelegate, TMulticastDelegateTraits<T>::GetName(), bFirstPropOfScriptStruct);
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        int32 FuncIdxInTable = GetDelegateInfo(L, IndexInStack, Object, CallbackFunction);      // get target UObject and Lua function
        if (FuncIdxInTable != INDEX_NONE)
        {
            T *ScriptDelegate = (T*)ValuePtr;
            FCallbackDesc Callback(Object->GetClass(), CallbackFunction, Object);
            FName FuncName = FDelegateHelper::GetBindedFunctionName(Callback);
            if (FuncName == NAME_None)
            {
                // no delegate function is created yet
                lua_rawgeti(L, IndexInStack, FuncIdxInTable);
                int32 CallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
                FDelegateHelper::Add(ScriptDelegate, MulticastDelegateProperty, Object, Callback, CallbackRef);
            }
            else
            {
				UE_LOG(UnLuaDelegate, Verbose, TEXT("++ %d %s %p %s"), FDelegateHelper::GetNumBindings(Callback), *Object->GetName(), Object, *FuncName.ToString());
                // a delegate function is created already
                FScriptDelegate DynamicDelegate;
                DynamicDelegate.BindUFunction(Object, FuncName);
                TMulticastDelegateTraits<T>::AddDelegate(MulticastDelegateProperty, DynamicDelegate, ValuePtr);
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TTABLE)
            {
                ErrorMsg = FString::Printf(TEXT("table needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
                return false;
            }
        }

        return true;
    };
#endif
};


/**
 * Create a property descriptor
 */
FPropertyDesc* FPropertyDesc::Create(FProperty *InProperty)
{
    // #lizard forgives

    FPropertyDesc* PropertyDesc = nullptr;
    int32 Type = ::GetPropertyType(InProperty);
    switch (Type)
    {
    case CPT_Byte:
    {
        const FByteProperty* TempByteProperty = CastField<FByteProperty>(InProperty);
        if (TempByteProperty->Enum)
        {
            RegisterEnum(UnLua::GetState(), TempByteProperty->Enum);
        }
    }
    case CPT_Int8:
    case CPT_Int16:
    case CPT_Int:
    case CPT_Int64:
    case CPT_UInt16:
    case CPT_UInt32:
    case CPT_UInt64:
        {
		    PropertyDesc = new FIntegerPropertyDesc(InProperty);
		    break;
        }
    case CPT_Float:
    case CPT_Double:
        {
            PropertyDesc = new FFloatPropertyDesc(InProperty);
            break;
        }
    case CPT_Enum:
        {
            PropertyDesc = new FEnumPropertyDesc(InProperty);
            break;
        }
    case CPT_Bool:
        {
		    PropertyDesc = new FBoolPropertyDesc(InProperty);
		    break;
        }
    case CPT_ObjectReference:
    case CPT_WeakObjectReference:
    case CPT_LazyObjectReference:
        {
		    PropertyDesc = new FObjectPropertyDesc(InProperty, false);
		    break;
        }
    case CPT_SoftObjectReference:
        {
            PropertyDesc = new FSoftObjectPropertyDesc(InProperty);
			//PropertyDesc = new FObjectPropertyDesc(InProperty, true);
            break;
        }
	case CPT_Interface:
        {
            PropertyDesc = new FInterfacePropertyDesc(InProperty);
            break;
        }
    case CPT_Name:
        {
            PropertyDesc = new FNamePropertyDesc(InProperty);
            break;
        }

    case CPT_String:
        {
            PropertyDesc = new FStringPropertyDesc(InProperty);
            break;
        }

    case CPT_Text:
        {
            PropertyDesc = new FTextPropertyDesc(InProperty);
            break;
        }

    case CPT_Array:
        {
            PropertyDesc = new FArrayPropertyDesc(InProperty);
            break;
        }
    case CPT_Map:
        {
            PropertyDesc = new FMapPropertyDesc(InProperty);
            break;
        }
    case CPT_Set:
        {
            PropertyDesc = new FSetPropertyDesc(InProperty);
            break;
        }
    case CPT_Struct:
        {
            PropertyDesc = new FScriptStructPropertyDesc(InProperty);
            break;
        }
    case CPT_Delegate:
        {
            PropertyDesc = new FDelegatePropertyDesc(InProperty);
            break;
        }
    case CPT_MulticastDelegate:
        {
            PropertyDesc = new TMulticastDelegatePropertyDesc<FMulticastScriptDelegate>(InProperty);
            break;
        }
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
    case CPT_MulticastSparseDelegate:
        {
            PropertyDesc = new TMulticastDelegatePropertyDesc<FSparseDelegate>(InProperty);
            break;
        }
#endif
    }
	
    if (PropertyDesc)
    {
        PropertyDesc->SetPropertyType(Type);
    }
    return PropertyDesc;
}

/**
 * Get type of a FProperty
 */
int32 GetPropertyType(const FProperty *Property)
{
    // #lizard forgives

    int32 Type = CPT_None;
    if (Property)
    {
        if (const FByteProperty *TempByteProperty = CastField<FByteProperty>(Property))
        {
            Type = CPT_Byte;
        }
        else if (const FInt8Property *TempI8Property = CastField<FInt8Property>(Property))
        {
            Type = CPT_Int8;
        }
        else if (const FInt16Property *TempI16Property = CastField<FInt16Property>(Property))
        {
            Type = CPT_Int16;
        }
        else if (const FIntProperty *TempI32Property = CastField<FIntProperty>(Property))
        {
            Type = CPT_Int;
        }
        else if (const FInt64Property *TempI64Property = CastField<FInt64Property>(Property))
        {
            Type = CPT_Int64;
        }
        else if (const FUInt16Property *TempU16Property = CastField<FUInt16Property>(Property))
        {
            Type = CPT_UInt16;
        }
        else if (const FUInt32Property *TempU32Property = CastField<FUInt32Property>(Property))
        {
            Type = CPT_UInt32;
        }
        else if (const FUInt64Property *TempU64Property = CastField<FUInt64Property>(Property))
        {
            Type = CPT_UInt64;
        }
        else if (const FFloatProperty *TempFloatProperty = CastField<FFloatProperty>(Property))
        {
            Type = CPT_Float;
        }
        else if (const FDoubleProperty *TempDoubleProperty = CastField<FDoubleProperty>(Property))
        {
            Type = CPT_Double;
        }
        else if (const FEnumProperty *TempEnumProperty = CastField<FEnumProperty>(Property))
        {
            Type = CPT_Enum;
        }
        else if (const FBoolProperty *TempBoolProperty = CastField<FBoolProperty>(Property))
        {
            Type = CPT_Bool;
        }
        else if (const FObjectProperty *TempObjectProperty = CastField<FObjectProperty>(Property))
        {
            Type = CPT_ObjectReference;
        }
        else if (const FWeakObjectProperty *TempWeakObjectProperty = CastField<FWeakObjectProperty>(Property))
        {
            Type = CPT_WeakObjectReference;
        }
        else if (const FLazyObjectProperty *TempLazyObjectProperty = CastField<FLazyObjectProperty>(Property))
        {
            Type = CPT_LazyObjectReference;
        }
        else if (const FSoftObjectProperty *TempSoftObjectProperty = CastField<FSoftObjectProperty>(Property))
        {
            Type = CPT_SoftObjectReference;
        }
        else if (const FInterfaceProperty *TempInterfaceProperty = CastField<FInterfaceProperty>(Property))
        {
            Type = CPT_Interface;
        }
        else if (const FNameProperty *TempNameProperty = CastField<FNameProperty>(Property))
        {
            Type = CPT_Name;
        }
        else if (const FStrProperty *TempStringProperty = CastField<FStrProperty>(Property))
        {
            Type = CPT_String;
        }
        else if (const FTextProperty *TempTextProperty = CastField<FTextProperty>(Property))
        {
            Type = CPT_Text;
        }
        else if (const FArrayProperty *TempArrayProperty = CastField<FArrayProperty>(Property))
        {
            Type = CPT_Array;
        }
        else if (const FMapProperty *TempMapProperty = CastField<FMapProperty>(Property))
        {
            Type = CPT_Map;
        }
        else if (const FSetProperty *TempSetProperty = CastField<FSetProperty>(Property))
        {
            Type = CPT_Set;
        }
        else if (const FStructProperty *TempStructProperty = CastField<FStructProperty>(Property))
        {
            Type = CPT_Struct;
        }
        else if (const FDelegateProperty *TempDelegateProperty = CastField<FDelegateProperty>(Property))
        {
            Type = CPT_Delegate;
        }
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 23
        else if (const FMulticastDelegateProperty *TempMulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Property))
        {
            Type = CPT_MulticastDelegate;
        }
#else
        else if (const FMulticastInlineDelegateProperty *TempMulticastInlineDelegateProperty = CastField<FMulticastInlineDelegateProperty>(Property))
        {
            Type = CPT_MulticastDelegate;
        }
        else if (const FMulticastSparseDelegateProperty *TempMulticastSparseDelegateProperty = CastField<FMulticastSparseDelegateProperty>(Property))
        {
            Type = CPT_MulticastSparseDelegate;
        }
#endif
    }
    return Type;
}
