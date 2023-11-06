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
#include "LowLevel.h"
#include "LuaCore.h"
#include "LuaEnv.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"
#include "ObjectReferencer.h"

FPropertyDesc::FPropertyDesc(FProperty *InProperty) : Property(InProperty) 
{
    PropertyType = CPT_None;
    PropertyPtr = InProperty;
    Name = Property->GetName();
}

bool FPropertyDesc::IsValid() const
{
    if (!PropertyPtr.IsValid())
        return false;
    
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 25
    return UnLua::IsUObjectValid(Property);
#else
    bool bValid = true;

    switch (PropertyType)
    {
    case CPT_Interface:
        {
            bValid = UnLua::IsUObjectValid(((FInterfaceProperty*)Property)->InterfaceClass);
            break;
        }
    case CPT_Delegate:
        {
            bValid = UnLua::IsUObjectValid(((FDelegateProperty*)Property)->SignatureFunction);
            break;
        }
    case CPT_MulticastDelegate:
    case CPT_MulticastSparseDelegate:
        {
            bValid = UnLua::IsUObjectValid(((FMulticastDelegateProperty*)Property)->SignatureFunction);
            break;
        }
    case CPT_Struct:
        {
            bValid = UnLua::IsUObjectValid(((FStructProperty*)Property)->Struct);
            break;
        }
    case CPT_ObjectReference:
    case CPT_WeakObjectReference:
    case CPT_LazyObjectReference:
    case CPT_SoftObjectReference:
        {
            bValid = UnLua::IsUObjectValid(((FObjectPropertyBase*)Property)->PropertyClass);
            break;
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
        if (!IsOutParameter())
            return false;

        bool bTwoLvlPtr;
        void** Userdata = (void**)UnLua::LowLevel::GetUserdata(L, DestIndexInStack, &bTwoLvlPtr);
        if (!bTwoLvlPtr || !Userdata)
            return false;

        *Userdata = ObjectBaseProperty->GetObjectPropertyValue(Property->ContainerPtrToValuePtr<void>(SrcContainerPtr));
        return true;
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
        auto Object = UnLua::GetUObject(L, IndexInStack, false);
        if (UnLua::LowLevel::IsReleasedPtr(Object))
        {
            UNLUA_LOGWARNING(L, LogUnLua, Warning, TEXT("attempt to set property %s with released object"), *GetName());
            Object = nullptr;
        }

        if (MetaClass)
        {
            UClass *Class = Cast<UClass>(Object);
            if (Class && !Class->IsChildOf(MetaClass))
            {
                Class = nullptr;
            }
            ObjectBaseProperty->SetObjectPropertyValue(ValuePtr, Object);
        }
        else
        {
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
        UnLua::FAutoStack AutoStack(L);
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
        UnLua::FAutoStack AutoStack(L);

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
        UnLua::FAutoStack AutoStack(L);

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
        NameProperty->SetPropertyValue(ValuePtr, FName(UTF8_TO_TCHAR(lua_tostring(L, IndexInStack))));
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
#if UNLUA_ENABLE_FTEXT
            const auto Text = TextProperty->GetPropertyValue(ValuePtr);
            const auto Userdata = NewTypedUserdata(L, FText);
            const auto NewTextPtr = new(Userdata) FText;
            *NewTextPtr = Text;
#else
            lua_pushstring(L, TCHAR_TO_UTF8(*TextProperty->GetPropertyValue(ValuePtr).ToString()));
#endif
        }
    }

    virtual bool SetValueInternal(lua_State* L, void* ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
#if UNLUA_ENABLE_FTEXT
        TextProperty->SetPropertyValue(ValuePtr, *(FText*)GetCppInstanceFast(L, IndexInStack));
#else
        TextProperty->SetPropertyValue(ValuePtr, FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, IndexInStack))));
#endif
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        const auto Type = lua_type(L, IndexInStack);
#if UNLUA_ENABLE_FTEXT
        if (Type != LUA_TUSERDATA)
        {
            ErrorMsg = FString::Printf(TEXT("userdata is needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
            return false;
        }
        return true;        
#else
        if (Type == LUA_TNIL || (Type == LUA_TSTRING || Type == LUA_TNUMBER))
            return true;
        ErrorMsg = FString::Printf(TEXT("string needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
        return false;
#endif
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
        const auto& Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        FScriptArray *ScriptArray;
        if (bCreateCopy)
        {
            const auto LuaArray = Registry->NewArray(L, InnerProperty, FLuaArray::OwnedBySelf); 
            ScriptArray = LuaArray->GetContainerPtr();
            ArrayProperty->CopyCompleteValue(ScriptArray, ValuePtr);
        }
        else
        {
            ScriptArray = (FScriptArray*)(&ArrayProperty->GetPropertyValue(ValuePtr));
            Registry->FindOrAdd(L, ScriptArray, InnerProperty);
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
            FLuaArray* Src = (FLuaArray*)lua_touserdata(L, IndexInStack);
            if (Src)
            {
                if (!bCopyValue && Property->HasAnyPropertyFlags(CPF_OutParm))
                {
                    if (Src->ElementSize < ArrayProperty->Inner->ElementSize)
                    {
                        FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
                        if (Src->Num() > 0)
                        {
                            Helper.AddValues(Src->Num());
                            for (int32 ArrayIndex = 0; ArrayIndex < Src->Num(); ArrayIndex++)
                            {
                                void* Dst = Helper.GetRawPtr(ArrayIndex);
                                Src->Get(ArrayIndex, Dst);
                            }
                        }
                    }
                    else
                    {
                        FMemory::Memcpy(ValuePtr, Src->ScriptArray, sizeof(FScriptArray)); // shallow copy
                    }
                    return false;
                }
                else
                {
                    ArrayProperty->CopyCompleteValue(ValuePtr, Src->ScriptArray);
                    //ArrayProperty->SetPropertyValue(ValuePtr, *LuaArray->ScriptArray);    // copy constructor of FScriptArray doesn't work
                }
            }
        }
        return true;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        UnLua::FAutoStack AutoStack(L);

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
        Array->Inner->WriteValue_InContainer(L, Data, -1);
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
        const auto& Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        FScriptMap *ScriptMap;
        if (bCreateCopy)
        {
            const auto LuaMap = Registry->NewMap(L, KeyProperty, ValueProperty, FLuaMap::OwnedBySelf);
            ScriptMap = LuaMap->GetContainerPtr();
            MapProperty->CopyCompleteValue(ScriptMap, ValuePtr);
        }
        else
        {
            ScriptMap = (FScriptMap*)&MapProperty->GetPropertyValue(ValuePtr);
            Registry->FindOrAdd(L, ScriptMap, KeyProperty, ValueProperty);
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
        UnLua::FAutoStack AutoStack(L);

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
        Map->KeyInterface->WriteValue_InContainer(L, Map->ElementCache, -2);
        Map->ValueInterface->WriteValue_InContainer(L, Map->ValueInterface->GetOffset() > 0 ? Map->ElementCache : ValueCache, -1);
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
        const auto& Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
        FScriptSet *ScriptSet;
        if (bCreateCopy)
        {
            const auto LuaSet = Registry->NewSet(L, InnerProperty, FLuaSet::OwnedBySelf); 
            ScriptSet = LuaSet->GetContainerPtr();
            SetProperty->CopyCompleteValue(ScriptSet, ValuePtr);
        }
        else
        {
            ScriptSet = (FScriptSet*)(&SetProperty->GetPropertyValue(ValuePtr));
            Registry->FindOrAdd(L, ScriptSet, InnerProperty);
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
        UnLua::FAutoStack AutoStack(L);

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
        Set->ElementInterface->WriteValue_InContainer(L, Set->ElementCache, -1);
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
        : FStructPropertyDesc(InProperty), StructName(*UnLua::LowLevel::GetMetatableName(StructProperty->Struct))
    {
        const auto ScriptStruct = CastChecked<UScriptStruct>(StructProperty->Struct);
        const auto CppStructOps = ScriptStruct->GetCppStructOps();
        StructSize = CppStructOps ? CppStructOps->GetSize() : ScriptStruct->GetStructureSize();
        UserdataPadding = UnLua::LowLevel::CalculateUserdataPadding(StructProperty->Struct);
    }

    virtual int32 GetSize() const override
    {
        return StructSize;
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
            StructProperty->CopySingleValue(Dest, Src);
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
        UnLua::FAutoStack AutoStack(L);
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

            FClassDesc* CurrentClassDesc = UnLua::FLuaEnv::FindEnv(L)->GetClassRegistry()->Find(MetatableName);
            if (!CurrentClassDesc)
            {
                ErrorMsg = FString::Printf(TEXT("metatable of userdata needed in registry but got no found"));
                return false;
            }

            UScriptStruct* ScriptStruct = CurrentClassDesc->AsScriptStruct();
            if (!ScriptStruct || !ScriptStruct->IsChildOf(StructProperty->Struct))
            {
                ErrorMsg = FString::Printf(TEXT("struct %s needed but got %s"), *StructProperty->Struct->GetName(), ScriptStruct? *ScriptStruct->GetName(): TEXT("nil"));
                return false;
            }
        }

        return true;
    };
#endif

private:
    FTCHARToUTF8 StructName;
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

        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        FScriptDelegate* ScriptDelegate = (FScriptDelegate*)DelegateProperty->GetPropertyValuePtr(ValuePtr);
        UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Register(ScriptDelegate, DelegateProperty, ScriptDelegate->GetUObject());
        GetValueInternal(L, ScriptDelegate, bCreateCopy);
    }

    virtual bool WriteValue_InContainer(lua_State *L, void *ContainerPtr, int32 IndexInStack, bool bCreateCopy) const override
    {
        if (UNLIKELY(!PropertyPtr.IsValid()))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write invalid property '%s'"), *Name);
            return false;
        }

        if (UnLua::LowLevel::IsReleasedPtr(ContainerPtr))
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to write property '%s' on released object"), *Name);
            return false;
        }

        auto DelegateRegistry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
        void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        FScriptDelegate* ScriptDelegate = DelegateProperty->GetPropertyValuePtr(ValuePtr);

        // https://github.com/Tencent/UnLua/issues/566
        if (Property->GetOwner<UFunction>())
        {
            ValuePtr = DelegateRegistry->Register(ScriptDelegate, DelegateProperty);
            const auto Ret = SetValueInternal(L, ValuePtr, IndexInStack, bCreateCopy);
            *ScriptDelegate = *(FScriptDelegate*)ValuePtr;
            return Ret;
        }

        DelegateRegistry->Register(ScriptDelegate, DelegateProperty, ScriptDelegate->GetUObject());
        return SetValueInternal(L, ValuePtr, IndexInStack, bCreateCopy);
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
            UnLua::PushPointer(L, ScriptDelegate, "FScriptDelegate", bFirstPropOfScriptStruct);
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        if (lua_isfunction(L, IndexInStack))
        {
            FScriptDelegate *Delegate = DelegateProperty->GetPropertyValuePtr(ValuePtr);
            UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Bind(L, IndexInStack, Delegate, Object);
            return bCopyValue;
        }

        int32 FuncIdxInTable = GetDelegateInfo(L, IndexInStack, Object, CallbackFunction);      // get target UObject and Lua function
        if (FuncIdxInTable != INDEX_NONE)
        {
            FScriptDelegate *Delegate = DelegateProperty->GetPropertyValuePtr(ValuePtr);
            lua_rawgeti(L, IndexInStack, FuncIdxInTable);
            UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Bind(L, -1, Delegate, Object);
            lua_pop(L, 1);
        }
        return bCopyValue;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TTABLE && Type != LUA_TFUNCTION)
            {
                ErrorMsg = FString::Printf(TEXT("table or function needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
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

        void* ValuePtr = (void*)Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        UObject* Owner = Property->GetOwnerStruct()->IsA<UClass>() ? (UObject*)ContainerPtr : nullptr;
        UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Register(ValuePtr, DelegateProperty, Owner);
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

        void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
        UObject* Owner = Property->GetOwnerStruct()->IsA<UClass>() ? (UObject*)ContainerPtr : nullptr;
        UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Register(ValuePtr, DelegateProperty, Owner);
        return SetValueInternal(L, ValuePtr, IndexInStack, bCreateCopy);
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
            UnLua::PushPointer(L, ScriptDelegate, TMulticastDelegateTraits<T>::GetName(), bFirstPropOfScriptStruct);
        }
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        UObject *Object = nullptr;
        const void *CallbackFunction = nullptr;
        if (lua_isfunction(L, IndexInStack))
        {
            FScriptDelegate *Delegate = DelegateProperty->GetPropertyValuePtr(ValuePtr);
            UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry()->Bind(L, IndexInStack, Delegate, Object);
            return bCopyValue;
        }
        
        int32 FuncIdxInTable = GetDelegateInfo(L, IndexInStack, Object, CallbackFunction);      // get target UObject and Lua function

        if (FuncIdxInTable != INDEX_NONE)
        {
            T *ScriptDelegate = (T*)ValuePtr;
            const auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetDelegateRegistry();
            Registry->Register(ScriptDelegate, MulticastDelegateProperty, Object);
            lua_rawgeti(L, IndexInStack, FuncIdxInTable);
            Registry->Add(L, -1, ScriptDelegate, Object);
            lua_pop(L, 1);
        }
        return bCopyValue;
    }

#if ENABLE_TYPE_CHECK == 1
    virtual bool CheckPropertyType(lua_State* L, int32 IndexInStack, FString& ErrorMsg, void* UserData)
    {
        int32 Type = lua_type(L, IndexInStack);
        if (Type != LUA_TNIL)
        {
            if (Type != LUA_TTABLE && Type != LUA_TFUNCTION)
            {
                ErrorMsg = FString::Printf(TEXT("table or function needed but got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
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
