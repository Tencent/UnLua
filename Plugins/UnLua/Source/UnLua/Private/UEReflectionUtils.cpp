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

#include "UEReflectionUtils.h"
#include "LuaCore.h"
#include "LuaContext.h"
#include "LuaSet.h"
#include "LuaMap.h"
#include "DelegateHelper.h"
#include "DefaultParamCollection.h"

/**
 * Integer property descriptor
 */
class FIntegerPropertyDesc : public FPropertyDesc
{
public:
    explicit FIntegerPropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

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
};

/**
 * Float property descriptor
 */
class FFloatPropertyDesc : public FPropertyDesc
{
public:
    explicit FFloatPropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

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
};

/**
 * Enum property descriptor
 */
class FEnumPropertyDesc : public FPropertyDesc
{
public:
    explicit FEnumPropertyDesc(UProperty *InProperty)
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
};

/**
 * Bool property descriptor
 */
class FBoolPropertyDesc : public FPropertyDesc
{
public:
    explicit FBoolPropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
    {
        lua_pushboolean(L, BoolProperty->GetPropertyValue(ValuePtr));
    }

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        BoolProperty->SetPropertyValue(ValuePtr, lua_toboolean(L, IndexInStack) != 0);
        return false;
    }
};

/**
 * UObject property descriptor
 */
class FObjectPropertyDesc : public FPropertyDesc
{
public:
    explicit FObjectPropertyDesc(UProperty *InProperty, bool bSoftObject)
        : FPropertyDesc(InProperty), MetaClass(nullptr)
    {
        if (ObjectBaseProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
        {
            MetaClass = bSoftObject ? (((USoftClassProperty*)Property)->MetaClass) : ((UClassProperty*)Property)->MetaClass;
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
            UnLua::PushUObject(L, ObjectBaseProperty->GetObjectPropertyValue(ValuePtr));
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
            ObjectBaseProperty->SetObjectPropertyValue(ValuePtr, UnLua::GetUObject(L, IndexInStack));
        }
        return true;
    }

private:
    UClass *MetaClass;
};

/**
 * Interface property descriptor
 */
class FInterfacePropertyDesc : public FPropertyDesc
{
public:
    explicit FInterfacePropertyDesc(UProperty *InProperty)
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
};

/**
 * Name property descriptor
 */
class FNamePropertyDesc : public FPropertyDesc
{
public:
    explicit FNamePropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

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
};

/**
 * String property descriptor
 */
class FStringPropertyDesc : public FPropertyDesc
{
public:
    explicit FStringPropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

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
};

/**
 * Text property descriptor
 */
class FTextPropertyDesc : public FPropertyDesc
{
public:
    explicit FTextPropertyDesc(UProperty *InProperty) : FPropertyDesc(InProperty) {}

    virtual void GetValueInternal(lua_State *L, const void *ValuePtr, bool bCreateCopy) const override
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

    virtual bool SetValueInternal(lua_State *L, void *ValuePtr, int32 IndexInStack, bool bCopyValue) const override
    {
        TextProperty->SetPropertyValue(ValuePtr, FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, IndexInStack))));
        return true;
    }
};

/**
 * TArray property descriptor
 */
class FArrayPropertyDesc : public FPropertyDesc
{
public:
    explicit FArrayPropertyDesc(UProperty *InProperty)
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
                FLuaArray *LuaArray = new(Userdata) FLuaArray(ScriptArray, InnerProperty, FLuaArray::OwnedByOther);
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
class FMapPropertyDesc : public FPropertyDesc
{
public:
    explicit FMapPropertyDesc(UProperty *InProperty)
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
                FLuaMap *LuaMap = new(Userdata) FLuaMap(ScriptMap, KeyProperty, ValueProperty, FLuaMap::OwnedByOther);
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
class FSetPropertyDesc : public FPropertyDesc
{
public:
    explicit FSetPropertyDesc(UProperty *InProperty)
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
                FLuaSet *LuaSet = new(Userdata) FLuaSet(ScriptSet, InnerProperty, FLuaSet::OwnedByOther);
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

class FStructPropertyDesc : public FPropertyDesc
{
public:
    explicit FStructPropertyDesc(UProperty *InProperty)
        : FPropertyDesc(InProperty), bFirstPropOfScriptStruct(Property->GetOuter()->IsA<UScriptStruct>() && Property->GetOffset_ForInternal() == 0)
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
    explicit FScriptStructPropertyDesc(UProperty *InProperty)
        : FStructPropertyDesc(InProperty), StructName(*FString::Printf(TEXT("F%s"), *StructProperty->Struct->GetName()))
    {
        FClassDesc *ClassDesc = RegisterClass(*GLuaCxt, StructProperty->Struct);    // register UScriptStruct first
        StructSize = ClassDesc->GetSize();
        UserdataPadding = ClassDesc->GetUserdataPadding();                          // padding size for userdata
    }

    FScriptStructPropertyDesc(UProperty *InProperty, bool bDynamicallyCreated)
        : FStructPropertyDesc(InProperty), StructName(*FString::Printf(TEXT("F%s"), *StructProperty->Struct->GetName()))
    {
        FClassDesc *ClassDesc = RegisterClass(*GLuaCxt, StructProperty->Struct);
        StructSize = ClassDesc->GetSize();
        UserdataPadding = ClassDesc->GetUserdataPadding();
        bFirstPropOfScriptStruct = false;
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
#if UE_BUILD_DEBUG
        int32 Type = lua_type(L, IndexInStack);
        if (Type == LUA_TUSERDATA)
        {
            int32 RetValue = lua_getmetatable(L, IndexInStack);
            if (RetValue != 1)
            {
                UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid meta table!"), ANSI_TO_TCHAR(__FUNCTION__));
                return false;
            }
            lua_pushstring(L, "__name");
            lua_rawget(L, -2);
            const char *MetatableName = lua_tostring(L, -1);
            if (FCStringAnsi::Strcmp(MetatableName, StructName.Get()))
            {
                FClassDesc *CurrentClassDesc = GReflectionRegistry.FindClass(MetatableName);
                check(CurrentClassDesc);
                UScriptStruct *ScriptStruct = CurrentClassDesc->AsScriptStruct();
                if (!ScriptStruct || !ScriptStruct->IsChildOf(StructProperty->Struct))
                {
                    lua_pop(L, 2);
                    UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("Mismatched meta table name, %s != %s"), ANSI_TO_TCHAR(StructName.Get()), ANSI_TO_TCHAR(MetatableName));
                    return false;
                }
            }
            lua_pop(L, 2);
        }
#endif
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
    explicit FDelegatePropertyDesc(UProperty *InProperty) : FStructPropertyDesc(InProperty) {}

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
            FCallbackDesc Callback(Object->GetClass(), CallbackFunction);
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
                ScriptDelegate->BindUFunction(Object, FuncName);        // a delegate function is created already
            }
        }
        return true;
    }
};

/**
 * Multicast delegate property descriptor
 */
template <typename T>
class TMulticastDelegatePropertyDesc : public FStructPropertyDesc
{
public:
    explicit TMulticastDelegatePropertyDesc(UProperty *InProperty) : FStructPropertyDesc(InProperty) {}

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
            FCallbackDesc Callback(Object->GetClass(), CallbackFunction);
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
                // a delegate function is created already
                FScriptDelegate DynamicDelegate;
                DynamicDelegate.BindUFunction(Object, FuncName);
                TMulticastDelegateTraits<T>::AddDelegate(MulticastDelegateProperty, DynamicDelegate, ValuePtr);
            }
        }
        return true;
    }
};


/**
 * Create a property descriptor
 */
FPropertyDesc* FPropertyDesc::Create(UProperty *InProperty)
{
    // #lizard forgives

    int32 Type = GetPropertyType(InProperty);
    switch (Type)
    {
    case CPT_Byte:
    case CPT_Int8:
    case CPT_Int16:
    case CPT_Int:
    case CPT_Int64:
    case CPT_UInt16:
    case CPT_UInt32:
    case CPT_UInt64:
        return new FIntegerPropertyDesc(InProperty);
    case CPT_Float:
    case CPT_Double:
        return new FFloatPropertyDesc(InProperty);
    case CPT_Enum:
        return new FEnumPropertyDesc(InProperty);
    case CPT_Bool:
        return new FBoolPropertyDesc(InProperty);
    case CPT_ObjectReference:
    case CPT_WeakObjectReference:
    case CPT_LazyObjectReference:
        return new FObjectPropertyDesc(InProperty, false);
    case CPT_SoftObjectReference:
        return new FObjectPropertyDesc(InProperty, true);
    case CPT_Interface:
        return new FInterfacePropertyDesc(InProperty);
    case CPT_Name:
        return new FNamePropertyDesc(InProperty);
    case CPT_String:
        return new FStringPropertyDesc(InProperty);
    case CPT_Text:
        return new FTextPropertyDesc(InProperty);
    case CPT_Array:
        return new FArrayPropertyDesc(InProperty);
    case CPT_Map:
        return new FMapPropertyDesc(InProperty);
    case CPT_Set:
        return new FSetPropertyDesc(InProperty);
    case CPT_Struct:
        return new FScriptStructPropertyDesc(InProperty);
    case CPT_Delegate:
        return new FDelegatePropertyDesc(InProperty);
    case CPT_MulticastDelegate:
        return new TMulticastDelegatePropertyDesc<FMulticastScriptDelegate>(InProperty);
#if ENGINE_MINOR_VERSION > 22
    case CPT_MulticastSparseDelegate:
        return new TMulticastDelegatePropertyDesc<FSparseDelegate>(InProperty);
#endif
    }
    return nullptr;
}


/**
 * Function descriptor constructor
 */
FFunctionDesc::FFunctionDesc(UFunction *InFunction, FParameterCollection *InDefaultParams, int32 InFunctionRef)
    : Function(InFunction), DefaultParams(InDefaultParams), ReturnPropertyIndex(INDEX_NONE), LatentPropertyIndex(INDEX_NONE)
    , FunctionRef(InFunctionRef), NumRefProperties(0), NumCalls(0), bStaticFunc(false), bInterfaceFunc(false)
{
    check(InFunction);

    FuncName = InFunction->GetName();

    bStaticFunc = InFunction->HasAnyFunctionFlags(FUNC_Static);         // a static function?

    UClass *OuterClass = InFunction->GetOuterUClass();
    if (OuterClass->HasAnyClassFlags(CLASS_Interface) && OuterClass != UInterface::StaticClass())
    {
        bInterfaceFunc = true;                                          // a function in interface?
    }

    // create persistent parameter buffer. memory for speed
#if ENABLE_PERSISTENT_PARAM_BUFFER
    Buffer = nullptr;
    if (InFunction->ParmsSize > 0)
    {
        Buffer = FMemory::Malloc(InFunction->ParmsSize, 16);
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        INC_MEMORY_STAT_BY(STAT_UnLua_PersistentParamBuffer_Memory, Size);
#endif
    }
#endif

    // pre-create OutParmRec. memory for speed
#if !SUPPORTS_RPC_CALL
    OutParmRec = nullptr;
    FOutParmRec *CurrentOutParmRec = nullptr;
#endif

    static const FName NAME_LatentInfo = TEXT("LatentInfo");
    Properties.Reserve(InFunction->NumParms);
    for (TFieldIterator<UProperty> It(InFunction); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        UProperty *Property = *It;
        int32 Index = Properties.Add(FPropertyDesc::Create(Property));
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            ReturnPropertyIndex = Index;                                // return property
        }
        else if (LatentPropertyIndex == INDEX_NONE && Property->GetFName() == NAME_LatentInfo)
        {
            LatentPropertyIndex = Index;                                // 'LatentInfo' property for latent function
        }
        else if (Property->HasAnyPropertyFlags(CPF_OutParm))
        {
            ++NumRefProperties;

            // pre-create OutParmRec for 'out' property
#if !SUPPORTS_RPC_CALL
            FOutParmRec *Out = (FOutParmRec*)FMemory::Malloc(sizeof(FOutParmRec), alignof(FOutParmRec));
#if STATS
            const uint32 Size = FMemory::GetAllocSize(Out);
            INC_MEMORY_STAT_BY(STAT_UnLua_OutParmRec_Memory, Size);
#endif
            Out->PropAddr = Property->ContainerPtrToValuePtr<uint8>(Buffer);
            Out->Property = Property;
            if (CurrentOutParmRec)
            {
                CurrentOutParmRec->NextOutParm = Out;
                CurrentOutParmRec = Out;
            }
            else
            {
                OutParmRec = Out;
                CurrentOutParmRec = Out;
            }
#endif

            if (!Property->HasAnyPropertyFlags(CPF_ConstParm))
            {
                OutPropertyIndices.Add(Index);                          // non-const reference property
            }
        }
    }

#if !SUPPORTS_RPC_CALL
    if (CurrentOutParmRec)
    {
        CurrentOutParmRec->NextOutParm = nullptr;
    }
#endif
}

/**
 * Function descriptor destructor
 */
FFunctionDesc::~FFunctionDesc()
{
    // free persistent parameter buffer
#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (Buffer)
    {
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        DEC_MEMORY_STAT_BY(STAT_UnLua_PersistentParamBuffer_Memory, Size);
#endif
        FMemory::Free(Buffer);
    }
#endif

    // free pre-created OutParmRec
#if !SUPPORTS_RPC_CALL
    while (OutParmRec)
    {
        FOutParmRec *NextOut = OutParmRec->NextOutParm;
#if STATS
        const uint32 Size = FMemory::GetAllocSize(OutParmRec);
        DEC_MEMORY_STAT_BY(STAT_UnLua_OutParmRec_Memory, Size);
#endif
        FMemory::Free(OutParmRec);
        OutParmRec = NextOut;
    }
#endif

    // release cached property descriptors
    for (FPropertyDesc *Property : Properties)
    {
        delete Property;
    }

    // remove Lua reference for this function
    if (FunctionRef != INDEX_NONE)
    {
        luaL_unref(*GLuaCxt, LUA_REGISTRYINDEX, FunctionRef);
    }
}

/**
 * Call Lua function that overrides this UFunction
 */
bool FFunctionDesc::CallLua(FFrame &Stack, void *RetValueAddress, bool bRpcCall, bool bUnpackParams)
{
    // push Lua function to the stack
    bool bSuccess = false;
    lua_State *L = *GLuaCxt;
    if (FunctionRef != INDEX_NONE)
    {
        bSuccess = PushFunction(L, Stack.Object, FunctionRef);
    }
    else
    {
        FunctionRef = PushFunction(L, Stack.Object, bRpcCall ? TCHAR_TO_ANSI(*FString::Printf(TEXT("%s_RPC"), *FuncName)) : TCHAR_TO_ANSI(*FuncName));
        bSuccess = FunctionRef != INDEX_NONE;
    }

    if (bSuccess)
    {
        if (bUnpackParams)
        {
#if ENABLE_PERSISTENT_PARAM_BUFFER
            void *Params = Buffer;
#else
            void *Params = Function->ParmsSize > 0 ? FMemory::Malloc(Function->ParmsSize, 16) : nullptr;
#endif

            for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & CPF_Parm) == CPF_Parm; ++It)
            {
                Stack.Step(Stack.Object, It->ContainerPtrToValuePtr<uint8>(Params));
            }
            check(Stack.PeekCode() == EX_EndFunctionParms);
            Stack.SkipCode(1);          // skip EX_EndFunctionParms

            bSuccess = CallLuaInternal(L, Params, Stack.OutParms, RetValueAddress);             // call Lua function...

#if !ENABLE_PERSISTENT_PARAM_BUFFER
            FMemory::Free(Params);
#endif
        }
        else
        {
            bSuccess = CallLuaInternal(L, Stack.Locals, Stack.OutParms, RetValueAddress);       // call Lua function...
        }
    }

    return bSuccess;
}

/**
 * Call the UFunction
 */
int32 FFunctionDesc::CallUE(lua_State *L, int32 NumParams, void *Userdata)
{
    check(Function);

    int32 FirstParamIndex = 1;
    UObject *Object = nullptr;
    if (bStaticFunc)
    {
        UClass *OuterClass = Function->GetOuterUClass();
        Object = OuterClass->GetDefaultObject();                // get CDO for static function
    }
    else
    {
        check(NumParams > 0);
        Object = UnLua::GetUObject(L, 1);
        ++FirstParamIndex;
        --NumParams;
    }

    if (!Object)
    {
        UE_LOG(LogUnLua, Warning, TEXT("!!! NULL target object for UFunction '%s'! Check the usage of ':' and '.'!"), *FuncName);
        return 0;
    }

#if SUPPORTS_RPC_CALL
    int32 Callspace = Object->GetFunctionCallspace(Function, nullptr);
    bool bRemote = Callspace & FunctionCallspace::Remote;
#else
    bool bRemote = false;
#endif

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags, Userdata);      // prepare values of properties

    UFunction *FinalFunction = Function;
    if (bInterfaceFunc)
    {
        // get target UFunction if it's a function in Interface
        FName FunctionName = Function->GetFName();
        FinalFunction = Object->GetClass()->FindFunctionByName(FunctionName);
        if (!FinalFunction)
        {
            UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("ERROR! Can't find UFunction '%s' in target object!"), *FuncName);
            return 0;
        }
#if UE_BUILD_DEBUG
        else if (FinalFunction != Function)
        {
            // todo: 'FinalFunction' must have the same signature with 'Function', check more parameters here
            check(FinalFunction->NumParms == Function->NumParms && FinalFunction->ParmsSize == Function->ParmsSize && FinalFunction->ReturnValueOffset == Function->ReturnValueOffset);
        }
#endif
    }
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    else
    {
        if (Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) && !bRemote)
        {
            UFunction *OverriddenFunc = GReflectionRegistry.FindOverriddenFunction(Function);
            if (OverriddenFunc)
            {
                FinalFunction = OverriddenFunc;
            }
        }
    }
#endif

    // call the UFuncton...
#if !SUPPORTS_RPC_CALL && !WITH_EDITOR
    if (FinalFunction->HasAnyFunctionFlags(FUNC_Native))
    {
        //FMemory::Memzero((uint8*)Params + FinalFunction->ParmsSize, FinalFunction->PropertiesSize - FinalFunction->ParmsSize);
        uint8* ReturnValueAddress = FinalFunction->ReturnValueOffset != MAX_uint16 ? (uint8*)Params + FinalFunction->ReturnValueOffset : nullptr;
        FFrame NewStack(Object, FinalFunction, Params, nullptr, Function->Children);
        NewStack.OutParms = OutParmRec;
        FinalFunction->Invoke(Object, NewStack, ReturnValueAddress);
    }
    else
#endif
    {
        if (bRemote)
        {
            Object->CallRemoteFunction(FinalFunction, Params, nullptr, nullptr);
        }
        else
        {
            Object->UObject::ProcessEvent(FinalFunction, Params);
        }
    }

    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // push 'out' properties to Lua stack
    return NumReturnValues;
}

/**
 * Fire a delegate
 */
int32 FFunctionDesc::ExecuteDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FScriptDelegate *ScriptDelegate)
{
    if (!ScriptDelegate || !ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid FScriptDelegate, or FScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags);
    ScriptDelegate->ProcessDelegate<UObject>(Params);
    int32 NumReturnValues = PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);
    return NumReturnValues;
}

/**
 * Fire a multicast delegate
 */
void FFunctionDesc::BroadcastMulticastDelegate(lua_State *L, int32 NumParams, int32 FirstParamIndex, FMulticastScriptDelegate *ScriptDelegate)
{
    if (!ScriptDelegate || !ScriptDelegate->IsBound())
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid FMulticastScriptDelegate, or FMulticastScriptDelegate is not bound!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    TArray<bool> CleanupFlags;
    CleanupFlags.AddZeroed(Properties.Num());
    void *Params = PreCall(L, NumParams, FirstParamIndex, CleanupFlags);
    ScriptDelegate->ProcessMulticastDelegate<UObject>(Params);
    PostCall(L, NumParams, FirstParamIndex, Params, CleanupFlags);      // !!! have no return values for multi-cast delegates
}

/**
 * Prepare values of properties for the UFunction
 */
void* FFunctionDesc::PreCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, TArray<bool> &CleanupFlags, void *Userdata)
{
    void *Params = nullptr;
#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (NumCalls < 1)
    {
        Params = Buffer;
    }
    else
#endif
    Params = Function->ParmsSize > 0 ? FMemory::Malloc(Function->ParmsSize, 16) : nullptr;

    ++NumCalls;

    int32 ParamIndex = 0;
    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        FPropertyDesc *Property = Properties[i];
        Property->InitializeValue(Params);
        if (i == LatentPropertyIndex)
        {
            // bind a callback to the latent function
            int32 ThreadRef = *((int32*)Userdata);
            FLatentActionInfo LatentActionInfo(ThreadRef, GetTypeHash(FGuid::NewGuid()), TEXT("OnLatentActionCompleted"), (UObject*)GLuaCxt->GetManager());
            Property->CopyValue(Params, &LatentActionInfo);
            continue;
        }
        if (i == ReturnPropertyIndex)
        {
            CleanupFlags[i] = ParamIndex < NumParams ? !Property->CopyBack(L, FirstParamIndex + ParamIndex, Params) : true;
            continue;
        }
        if (ParamIndex < NumParams)
        {
            CleanupFlags[i] = Property->SetValue(L, Params, FirstParamIndex + ParamIndex, false);
        }
        else if (!Property->IsOutParameter())
        {
            if (DefaultParams)
            {
                // set value for default parameter
                IParamValue **DefaultValue = DefaultParams->Parameters.Find(Property->GetProperty()->GetFName());
                if (DefaultValue)
                {
                    const void *ValuePtr = (*DefaultValue)->GetValue();
                    Property->CopyValue(Params, ValuePtr);
                    CleanupFlags[i] = true;
                }
            }
        }
        ++ParamIndex;
    }

    return Params;
}

/**
 * Handling 'out' properties
 */
int32 FFunctionDesc::PostCall(lua_State *L, int32 NumParams, int32 FirstParamIndex, void *Params, const TArray<bool> &CleanupFlags)
{
    int32 NumReturnValues = 0;

    for (int32 Index : OutPropertyIndices)
    {
        FPropertyDesc *Property = Properties[Index];
        if (Index >= NumParams || !Property->CopyBack(L, Params, FirstParamIndex + Index))
        {
            Property->GetValue(L, Params, true);
            ++NumReturnValues;
        }
    }

    if (ReturnPropertyIndex > INDEX_NONE)
    {
        FPropertyDesc *Property = Properties[ReturnPropertyIndex];
        if (!CleanupFlags[ReturnPropertyIndex])
        {
            int32 ReturnIndexInStack = FirstParamIndex + ReturnPropertyIndex;
            bool bResult = Property->CopyBack(L, Params, ReturnIndexInStack);
            check(bResult);
            lua_pushvalue(L, ReturnIndexInStack);
        }
        else
        {
            Property->GetValue(L, Params, true);
        }
        ++NumReturnValues;
    }

    for (int32 i = 0; i < Properties.Num(); ++i)
    {
        if (CleanupFlags[i])
        {
            Properties[i]->DestroyValue(Params);
        }
    }

    --NumCalls;

#if ENABLE_PERSISTENT_PARAM_BUFFER
    if (NumCalls > 0)
#endif
    FMemory::Free(Params);

    return NumReturnValues;
}

/**
 * Get OutParmRec for a non-const reference property
 */
static FOutParmRec* FindOutParmRec(FOutParmRec *OutParam, UProperty *OutProperty)
{
    while (OutParam)
    {
        if (OutParam->Property == OutProperty)
        {
            return OutParam;
        }
        OutParam = OutParam->NextOutParm;
    }
    return nullptr;
}

/**
 * Call Lua function that overrides this UFunction. 
 */
bool FFunctionDesc::CallLuaInternal(lua_State *L, void *InParams, FOutParmRec *OutParams, void *RetValueAddress) const
{
    // prepare parameters for Lua function
    FOutParmRec *OutParam = OutParams;
    for (const FPropertyDesc *Property : Properties)
    {
        if (Property->IsReturnParameter())
        {
            continue;
        }

        if (Property->IsConstOutParameter())
        {
            OutParam = FindOutParmRec(OutParam, Property->GetProperty());
            if (OutParam)
            {
                Property->GetValueInternal(L, OutParam->PropAddr, false);
                OutParam = OutParam->NextOutParm;
                continue;
            }
        }

        Property->GetValue(L, InParams, false);
    }

    int32 NumParams = HasReturnProperty() ? Properties.Num() : 1 + Properties.Num();
    int32 NumResult = GetNumOutProperties();
    bool bSuccess = CallFunction(L, NumParams, NumResult);      // pcall
    if (!bSuccess)
    {
        return false;
    }

    int32 OutPropertyIndex = -NumResult;
    OutParam = OutParams;
    for (int32 i = 0; i < OutPropertyIndices.Num(); ++i)
    {
        FPropertyDesc *OutProperty = Properties[OutPropertyIndices[i]];
        OutParam = FindOutParmRec(OutParam, OutProperty->GetProperty());
        check(OutParam);
        int32 Type = lua_type(L, OutPropertyIndex);
        if (Type == LUA_TNIL)
        {
            bSuccess = OutProperty->CopyBack(OutParam->PropAddr, OutProperty->GetProperty()->ContainerPtrToValuePtr<void>(InParams));   // copy back value to out property
            if (!bSuccess)
            {
                UNLUA_LOGERROR(L, LogUnLua, Error, TEXT("Can't copy value back!"));
            }
        }
        else
        {
            OutProperty->SetValueInternal(L, OutParam->PropAddr, OutPropertyIndex, true);       // set value for out property
        }
        OutParam = OutParam->NextOutParm;
        ++OutPropertyIndex;
    }
    if (ReturnPropertyIndex > INDEX_NONE)
    {
        check(RetValueAddress);
        Properties[ReturnPropertyIndex]->SetValueInternal(L, RetValueAddress, -1, true);        // set value for return property
    }

    lua_pop(L, NumResult);
    return true;
}


/**
 * Class descriptor constructor
 */
FClassDesc::FClassDesc(UStruct *InStruct, const FString &InName, EType InType)
    : Struct(InStruct), ClassName(InName), ClassFName(*InName), ClassAnsiName(*InName), Type(InType), UserdataPadding(0), Size(0), RefCount(0), Parent(nullptr), FunctionCollection(nullptr)
{
    if (InType == EType::CLASS)
    {
        Size = Struct->GetStructureSize();

        // register implemented interfaces
        for (FImplementedInterface &Interface : Class->Interfaces)
        {
            FClassDesc *InterfaceClass = GReflectionRegistry.RegisterClass(Interface.Class);
            RegisterClass(*GLuaCxt, Interface.Class);
        }

        FunctionCollection = GDefaultParamCollection.Find(ClassFName);
    }
    else if (InType == EType::SCRIPTSTRUCT)
    {
        UScriptStruct::ICppStructOps *CppStructOps = ScriptStruct->GetCppStructOps();
        int32 Alignment = CppStructOps ? CppStructOps->GetAlignment() : ScriptStruct->GetMinAlignment();
        Size = CppStructOps ? CppStructOps->GetSize() : ScriptStruct->GetStructureSize();
        UserdataPadding = CalcUserdataPadding(Alignment);       // calculate padding size for userdata
    }
}

/**
 * Class descriptor destructor
 */
FClassDesc::~FClassDesc()
{
    for (TMap<FName, FFieldDesc*>::TIterator It(Fields); It; ++It)
    {
        delete It.Value();
    }
    for (FPropertyDesc *Property : Properties)
    {
        delete Property;
    }
    for (FFunctionDesc *Function : Functions)
    {
        delete Function;
    }
}

/**
 * Release a class descriptor
 */
bool FClassDesc::Release(bool bKeepAlive)
{
    --RefCount;     // dec reference count
    if (!RefCount && !Struct->IsNative())
    {
        if (!bKeepAlive)
        {
            delete this;
            return true;
        }
    }
    return false;
}

/**
 * Reset a class descriptor
 */
void FClassDesc::Reset()
{
    Struct = nullptr;
    Type = EType::UNKNOWN;

    ClearLibrary(*GLuaCxt, ClassAnsiName.Get());            // clean up related Lua meta table
    ClearLoadedModule(*GLuaCxt, ClassAnsiName.Get());       // clean up required Lua module
}

/**
 * Register a field of this class
 */
FFieldDesc* FClassDesc::RegisterField(FName FieldName, FClassDesc *QueryClass)
{
    if (!Struct)
    {
        return nullptr;
    }

    FFieldDesc *FieldDesc = nullptr;
    FFieldDesc **FieldDescPtr = Fields.Find(FieldName);
    if (FieldDescPtr)
    {
        FieldDesc = *FieldDescPtr;
    }
    else
    {
        // a property or a function ?
        UProperty *Property = Struct->FindPropertyByName(FieldName);
        UFunction *Function = (!Property && Type == EType::CLASS) ? Class->FindFunctionByName(FieldName) : nullptr;
        UField *Field = Property ? (UField*)Property : Function;
        if (!Field && Type == EType::SCRIPTSTRUCT)
        {
            Property = Struct->CustomFindProperty(FieldName);
            Field = Property;
        }
        if (!Field)
        {
            return nullptr;
        }

        UStruct *OuterStruct = Cast<UStruct>(Field->GetOuter());
        if (OuterStruct)
        {
            if (OuterStruct != Struct)
            {
                FClassDesc *OuterClass = (FClassDesc*)GReflectionRegistry.RegisterClass(OuterStruct);
                check(OuterClass);
                return OuterClass->RegisterField(FieldName, QueryClass);
            }

            // create new Field descriptor
            FieldDesc = new FFieldDesc;
            FieldDesc->QueryClass = QueryClass;
            FieldDesc->OuterClass = this;
            Fields.Add(FieldName, FieldDesc);
            if (Property)
            {
                FieldDesc->FieldIndex = Properties.Add(FPropertyDesc::Create(Property));        // index of property descriptor
                ++FieldDesc->FieldIndex;
            }
            else
            {
                check(Function);
                FParameterCollection *DefaultParams = FunctionCollection ? FunctionCollection->Functions.Find(FieldName) : nullptr;
                FieldDesc->FieldIndex = Functions.Add(new FFunctionDesc(Function, DefaultParams, INDEX_NONE));  // index of function descriptor
                ++FieldDesc->FieldIndex;
                FieldDesc->FieldIndex = -FieldDesc->FieldIndex;
            }
        }
    }
    return FieldDesc;
}

/**
 * Get class inheritance chain
 */
void FClassDesc::GetInheritanceChain(TArray<FString> &NameChain, TArray<UStruct*> &StructChain) const
{
    check(Type != EType::UNKNOWN);
    UStruct *SuperStruct = Struct->GetInheritanceSuper();
    while (SuperStruct)
    {
        FString Name = FString::Printf(TEXT("%s%s"), SuperStruct->GetPrefixCPP(), *SuperStruct->GetName());
        NameChain.Add(Name);
        StructChain.Add(SuperStruct);
        SuperStruct = SuperStruct->GetInheritanceSuper();
    }
}


/**
 * Enum descriptor
 */
FEnumDesc::FEnumDesc(UEnum *InEnum)
    : Enum(InEnum), RefCount(0), Type(EType::Enum)
{
    if (Enum)
    {
        EnumName = Enum->GetName();
        UUserDefinedEnum *UDEnum = Cast<UUserDefinedEnum>(Enum);
        Type = UDEnum ? EType::UserDefinedEnum : EType::Enum;
    }
}

bool FEnumDesc::Release()
{
    --RefCount;
    if (!RefCount)
    {
        delete this;
        return true;
    }
    return false;
}

int64 FEnumDesc::GetEnumValue(UEnum *Enum, FName EntryName)
{
    check(Enum);
    return Enum->GetValueByName(EntryName);
}

int64 FEnumDesc::GetUserDefinedEnumValue(UEnum *Enum, FName EntryName)
{
    check(Enum);
    int32 NumEntries = Enum->NumEnums();
    for (int32 i = 0; i < NumEntries; ++i)
    {
        FName DisplayName(*Enum->GetDisplayNameTextByIndex(i).ToString());
        if (DisplayName == EntryName)
        {
            return Enum->GetValueByIndex(i);
        }
    }
    return INDEX_NONE;
}

/**
 * Clean up
 */
void FReflectionRegistry::Cleanup()
{
    for (TMap<FName, FClassDesc*>::TIterator It(Name2Classes); It; ++It)
    {
        delete It.Value();
    }
    for (TMap<FName, FEnumDesc*>::TIterator It(Enums); It; ++It)
    {
        delete It.Value();
    }
    for (TMap<UFunction*, FFunctionDesc*>::TIterator It(Functions); It; ++It)
    {
        delete It.Value();
    }
    Name2Classes.Empty();
    Struct2Classes.Empty();
    NonNativeStruct2Classes.Empty();
    Enums.Empty();
    Functions.Empty();
}

/**
 * Unregister a class
 */
bool FReflectionRegistry::UnRegisterClass(FClassDesc *ClassDesc)
{
    if (ClassDesc)
    {
        FName Name(ClassDesc->GetFName());
        UStruct *Struct = ClassDesc->AsStruct();
        if (!Struct)
        {
            delete ClassDesc;
            return true;
        }
        else if (ClassDesc->Release())
        {
            Name2Classes.Remove(Name);
            Struct2Classes.Remove(Struct);
            NonNativeStruct2Classes.Remove(Struct);
            return true;
        }
    }
    return false;
}

/**
 * Register a class 
 *
 * @param InName - class name
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(const TCHAR *InName, TArray<FClassDesc*> *OutChain)
{
    const TCHAR *Name = (InName[0] == 'U' || InName[0] == 'A' || InName[0] == 'F' || InName[0] == 'E') ? InName + 1 : InName;
    UStruct *Struct = FindObject<UStruct>(ANY_PACKAGE, Name);       // find first
    if (!Struct)
    {
        Struct = LoadObject<UStruct>(nullptr, Name);                // load if not found
    }
    return RegisterClass(Struct, OutChain);
}

/**
 * Register a class
 *
 * @param InStruct - UStruct
 * @param[out] OutChain - class inheritance chain
 * @return - the class descriptor
 */
FClassDesc* FReflectionRegistry::RegisterClass(UStruct *InStruct, TArray<FClassDesc*> *OutChain)
{
    if (!InStruct)
    {
        return nullptr;
    }

    FClassDesc::EType Type = FClassDesc::GetType(InStruct);
    if (Type == FClassDesc::EType::UNKNOWN)
    {
        return nullptr;
    }

    FClassDesc **ClassDescPtr = Struct2Classes.Find(InStruct);
    if (ClassDescPtr)       // already registered ?
    {
        GetClassChain(*ClassDescPtr, OutChain);
        return *ClassDescPtr;
    }

    FString ClassName = FString::Printf(TEXT("%s%s"), InStruct->GetPrefixCPP(), *InStruct->GetName());
    FClassDesc *ClassDesc = RegisterClassInternal(ClassName, InStruct, Type);
    GetClassChain(ClassDesc, OutChain);
    return ClassDesc;
}

bool FReflectionRegistry::NotifyUObjectDeleted(const UObjectBase *InObject)
{
    UStruct *Struct = (UStruct*)InObject;
    FClassDesc *ClassDesc = nullptr;
    bool bSuccess = NonNativeStruct2Classes.RemoveAndCopyValue(Struct, ClassDesc);
    if (bSuccess && ClassDesc)
    {
        UE_LOG(LogUnLua, Warning, TEXT("Class/ScriptStruct %s has been GCed by engine!!!"), *ClassDesc->GetName());
        ClassDesc->Reset();
        Name2Classes.Remove(ClassDesc->GetFName());
        Struct2Classes.Remove(Struct);
        return true;
    }
    return false;
}

/**
 * Register a UEnum
 */
FEnumDesc* FReflectionRegistry::RegisterEnum(const TCHAR *InName)
{
    FEnumDesc **EnumDescPtr = Enums.Find(FName(InName));
    if (EnumDescPtr)
    {
        return *EnumDescPtr;
    }

    UEnum *Enum = FindObject<UEnum>(ANY_PACKAGE, InName);
    if (!Enum)
    {
        Enum = LoadObject<UEnum>(nullptr, InName);
    }
    return RegisterEnum(Enum);
}

FEnumDesc* FReflectionRegistry::RegisterEnum(UEnum *InEnum)
{
    if (!InEnum)
    {
        return nullptr;
    }

    FName Name = InEnum->GetFName();
    FEnumDesc **EnumDescPtr = Enums.Find(Name);
    if (EnumDescPtr)
    {
        return *EnumDescPtr;
    }

    FEnumDesc *EnumDesc = new FEnumDesc(InEnum);
    Enums.Add(Name, EnumDesc);
    return EnumDesc;
}

/**
 * Register a UFunction
 */
FFunctionDesc* FReflectionRegistry::RegisterFunction(UFunction *InFunction, int32 InFunctionRef)
{
    FFunctionDesc *Function = nullptr;
    FFunctionDesc **FunctionPtr = Functions.Find(InFunction);
    if (FunctionPtr)
    {
        Function = *FunctionPtr;
        if (InFunctionRef != INDEX_NONE)
        {
            check(Function->FunctionRef == INDEX_NONE || Function->FunctionRef == InFunctionRef);
            Function->FunctionRef = InFunctionRef;
        }
    }
    else
    {
        Function = new FFunctionDesc(InFunction, nullptr, InFunctionRef);
        Functions.Add(InFunction, Function);
    }
    return Function;
}

bool FReflectionRegistry::UnRegisterFunction(UFunction *InFunction)
{
    FFunctionDesc *FunctionDesc = nullptr;
    if (Functions.RemoveAndCopyValue(InFunction, FunctionDesc))
    {
        delete FunctionDesc;
        return true;
    }
    return false;
}

#if ENABLE_CALL_OVERRIDDEN_FUNCTION
bool FReflectionRegistry::AddOverriddenFunction(UFunction *NewFunc, UFunction *OverriddenFunc)
{
    UFunction **OverriddenFuncPtr = OverriddenFunctions.Find(NewFunc);
    if (!OverriddenFuncPtr)
    {
        OverriddenFunctions.Add(NewFunc, OverriddenFunc);
        return true;
    }
    return false;
}

UFunction* FReflectionRegistry::RemoveOverriddenFunction(UFunction *NewFunc)
{
    UFunction *OverriddenFunc = nullptr;
    OverriddenFunctions.RemoveAndCopyValue(NewFunc, OverriddenFunc);
    return OverriddenFunc;
}

UFunction* FReflectionRegistry::FindOverriddenFunction(UFunction *NewFunc)
{
    UFunction **OverriddenFuncPtr = OverriddenFunctions.Find(NewFunc);
    return OverriddenFuncPtr ? *OverriddenFuncPtr : nullptr;
}
#endif

FClassDesc* FReflectionRegistry::RegisterClassInternal(const FString &ClassName, UStruct *Struct, FClassDesc::EType Type)
{
    check(Struct && Type != FClassDesc::EType::UNKNOWN);
    FClassDesc *ClassDesc = new FClassDesc(Struct, ClassName, Type);
    Name2Classes.Add(FName(*ClassName), ClassDesc);
    Struct2Classes.Add(Struct, ClassDesc);
    if (!ClassDesc->IsNative())
    {
        NonNativeStruct2Classes.Add(Struct, ClassDesc);
    }

    FClassDesc *CurrentClass = ClassDesc;
    TArray<FString> NameChain;
    TArray<UStruct*> StructChain;
    ClassDesc->GetInheritanceChain(NameChain, StructChain);
    for (int32 i = 0; i < NameChain.Num(); ++i)
    {
        FClassDesc **Class = Struct2Classes.Find(StructChain[i]);
        if (!Class)
        {
            CurrentClass->Parent = new FClassDesc(StructChain[i], NameChain[i], Type);
            Name2Classes.Add(*NameChain[i], CurrentClass->Parent);
            Struct2Classes.Add(StructChain[i], CurrentClass->Parent);
            if (!CurrentClass->Parent->IsNative())
            {
                NonNativeStruct2Classes.Add(StructChain[i], CurrentClass->Parent);
            }
        }
        else
        {
            CurrentClass->Parent = *Class;
            break;
        }
        CurrentClass = CurrentClass->Parent;
    }

    return ClassDesc;
}

/**
 * get class inheritance chain
 */
void FReflectionRegistry::GetClassChain(FClassDesc *ClassDesc, TArray<FClassDesc*> *OutChain)
{
    if (OutChain)
    {
        while (ClassDesc)
        {
            OutChain->Add(ClassDesc);
            ClassDesc = ClassDesc->Parent;
        }
    }
}

FReflectionRegistry GReflectionRegistry;        // global reflection registry


/**
 * Get type of a UProperty
 */
int32 GetPropertyType(const UProperty *Property)
{
    // #lizard forgives

    int32 Type = CPT_None;
    if (Property)
    {
        if (const UByteProperty *TempByteProperty = Cast<UByteProperty>(Property))
        {
            Type = CPT_Byte;
        }
        else if (const UInt8Property *TempI8Property = Cast<UInt8Property>(Property))
        {
            Type = CPT_Int8;
        }
        else if (const UInt16Property *TempI16Property = Cast<UInt16Property>(Property))
        {
            Type = CPT_Int16;
        }
        else if (const UIntProperty *TempI32Property = Cast<UIntProperty>(Property))
        {
            Type = CPT_Int;
        }
        else if (const UInt64Property *TempI64Property = Cast<UInt64Property>(Property))
        {
            Type = CPT_Int64;
        }
        else if (const UUInt16Property *TempU16Property = Cast<UUInt16Property>(Property))
        {
            Type = CPT_UInt16;
        }
        else if (const UUInt32Property *TempU32Property = Cast<UUInt32Property>(Property))
        {
            Type = CPT_UInt32;
        }
        else if (const UUInt64Property *TempU64Property = Cast<UUInt64Property>(Property))
        {
            Type = CPT_UInt64;
        }
        else if (const UFloatProperty *TempFloatProperty = Cast<UFloatProperty>(Property))
        {
            Type = CPT_Float;
        }
        else if (const UDoubleProperty *TempDoubleProperty = Cast<UDoubleProperty>(Property))
        {
            Type = CPT_Double;
        }
        else if (const UEnumProperty *TempEnumProperty = Cast<UEnumProperty>(Property))
        {
            Type = CPT_Enum;
        }
        else if (const UBoolProperty *TempBoolProperty = Cast<UBoolProperty>(Property))
        {
            Type = CPT_Bool;
        }
        else if (const UObjectProperty *TempObjectProperty = Cast<UObjectProperty>(Property))
        {
            Type = CPT_ObjectReference;
        }
        else if (const UWeakObjectProperty *TempWeakObjectProperty = Cast<UWeakObjectProperty>(Property))
        {
            Type = CPT_WeakObjectReference;
        }
        else if (const ULazyObjectProperty *TempLazyObjectProperty = Cast<ULazyObjectProperty>(Property))
        {
            Type = CPT_LazyObjectReference;
        }
        else if (const USoftObjectProperty *TempSoftObjectProperty = Cast<USoftObjectProperty>(Property))
        {
            Type = CPT_SoftObjectReference;
        }
        else if (const UInterfaceProperty *TempInterfaceProperty = Cast<UInterfaceProperty>(Property))
        {
            Type = CPT_Interface;
        }
        else if (const UNameProperty *TempNameProperty = Cast<UNameProperty>(Property))
        {
            Type = CPT_Name;
        }
        else if (const UStrProperty *TempStringProperty = Cast<UStrProperty>(Property))
        {
            Type = CPT_String;
        }
        else if (const UTextProperty *TempTextProperty = Cast<UTextProperty>(Property))
        {
            Type = CPT_Text;
        }
        else if (const UArrayProperty *TempArrayProperty = Cast<UArrayProperty>(Property))
        {
            Type = CPT_Array;
        }
        else if (const UMapProperty *TempMapProperty = Cast<UMapProperty>(Property))
        {
            Type = CPT_Map;
        }
        else if (const USetProperty *TempSetProperty = Cast<USetProperty>(Property))
        {
            Type = CPT_Set;
        }
        else if (const UStructProperty *TempStructProperty = Cast<UStructProperty>(Property))
        {
            Type = CPT_Struct;
        }
        else if (const UDelegateProperty *TempDelegateProperty = Cast<UDelegateProperty>(Property))
        {
            Type = CPT_Delegate;
        }
#if ENGINE_MINOR_VERSION < 23
        else if (const UMulticastDelegateProperty *TempMulticastDelegateProperty = Cast<UMulticastDelegateProperty>(Property))
        {
            Type = CPT_MulticastDelegate;
        }
#else
        else if (const UMulticastInlineDelegateProperty *TempMulticastInlineDelegateProperty = Cast<UMulticastInlineDelegateProperty>(Property))
        {
            Type = CPT_MulticastDelegate;
        }
        else if (const UMulticastSparseDelegateProperty *TempMulticastSparseDelegateProperty = Cast<UMulticastSparseDelegateProperty>(Property))
        {
            Type = CPT_MulticastSparseDelegate;
        }
#endif
    }
    return Type;
}
