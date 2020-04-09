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

#include "PropertyCreator.h"
#include "UEReflectionUtils.h"

class FPropertyCreator : public IPropertyCreator
{
public:
    FPropertyCreator()
        : ScriptStruct(nullptr)
    {
        ScriptStruct = FindObject<UScriptStruct>(ANY_PACKAGE, TEXT("PropertyCollector"));

        // ScriptStruct->CreateCluster() fail to create a cluster...
        int32 RootInternalIndex = GUObjectArray.ObjectToIndex(ScriptStruct);
        FUObjectItem *RootItem = GUObjectArray.IndexToObject(RootInternalIndex);
        if (RootItem->GetOwnerIndex() == 0 && !RootItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
        {
            int32 ClusterIndex = GUObjectClusters.AllocateCluster(RootInternalIndex);
            RootItem->SetClusterIndex(ClusterIndex);
            RootItem->SetFlags(EInternalObjectFlags::ClusterRoot);
        }
    }

    virtual ~FPropertyCreator()
    {
        BoolPropertyDesc.Reset();
        IntPropertyDesc.Reset();
        FloatPropertyDesc.Reset();
        StringPropertyDesc.Reset();
        NamePropertyDesc.Reset();
        TextPropertyDesc.Reset();

        CleanupProperties(EnumPropertyDescMap);
        CleanupProperties(ClassPropertyDescMap);
        CleanupProperties(ObjectPropertyDescMap);
        CleanupProperties(SoftClassPropertyDescMap);
        CleanupProperties(SoftObjectPropertyDescMap);
        CleanupProperties(WeakObjectPropertyDescMap);
        CleanupProperties(LazyObjectPropertyDescMap);
        CleanupProperties(StructPropertyDescMap);
    }

    virtual void Cleanup() override
    {
        // cleanup non-native properties
        CleanupNonNativeProperties(EnumPropertyDescMap);
        CleanupNonNativeProperties(ClassPropertyDescMap);
        CleanupNonNativeProperties(ObjectPropertyDescMap);
        CleanupNonNativeProperties(SoftClassPropertyDescMap);
        CleanupNonNativeProperties(SoftObjectPropertyDescMap);
        CleanupNonNativeProperties(WeakObjectPropertyDescMap);
        CleanupNonNativeProperties(LazyObjectPropertyDescMap);
        CleanupNonNativeProperties(StructPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateBoolProperty() override
    {
        if (!BoolPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UBoolProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UBoolProperty(FObjectInitializer(), EC_CppProperty, 0, (EPropertyFlags)0, 0xFF, 1, true);
            BoolPropertyDesc = OnPropertyCreated(Property);
        }
        return BoolPropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateIntProperty() override
    {
        if (!IntPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UIntProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UIntProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            IntPropertyDesc = OnPropertyCreated(Property);
        }
        return IntPropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateFloatProperty() override
    {
        if (!FloatPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UFloatProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UFloatProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            FloatPropertyDesc = OnPropertyCreated(Property);
        }
        return FloatPropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateStringProperty() override
    {
        if (!StringPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UStrProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UStrProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            StringPropertyDesc = OnPropertyCreated(Property);
        }
        return StringPropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateNameProperty() override
    {
        if (!NamePropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UNameProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UNameProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            NamePropertyDesc = OnPropertyCreated(Property);
        }
        return NamePropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateTextProperty() override
    {
        if (!TextPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UTextProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UTextProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            TextPropertyDesc = OnPropertyCreated(Property);
        }
        return TextPropertyDesc;
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateEnumProperty(UEnum *Enum) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = EnumPropertyDescMap.Find(Enum);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UEnumProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UEnumProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Enum);
        UNumericProperty *UnderlyingProp = NewObject<UByteProperty>(Property, TEXT("UnderlyingType"));
        Property->AddCppProperty(UnderlyingProp);
        FArchive Ar;
        Property->Link(Ar);
        return OnPropertyCreated(Property, Enum, EnumPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateClassProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = ClassPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSubclassOf<...>
        UClassProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UClassProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class, nullptr);
        return OnPropertyCreated(Property, Class, ClassPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateObjectProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = ObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Class);
        return OnPropertyCreated(Property, Class, ObjectPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateSoftClassProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = SoftClassPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSoftClassPtr<...>
        USoftClassProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) USoftClassProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        return OnPropertyCreated(Property, Class, SoftClassPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateSoftObjectProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = SoftObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSoftObjectPtr<...>
        USoftObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) USoftObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        return OnPropertyCreated(Property, Class, SoftObjectPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateWeakObjectProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = WeakObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TWeakObjectPtr<...>
        UWeakObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UWeakObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        return OnPropertyCreated(Property, Class, WeakObjectPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateLazyObjectProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = LazyObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TLazyObjectPtr<...>
        ULazyObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) ULazyObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        return OnPropertyCreated(Property, Class, LazyObjectPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateInterfaceProperty(UClass *Class) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = InterfacePropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TScriptInterface<...>
        UInterfaceProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UInterfaceProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        return OnPropertyCreated(Property, Class, InterfacePropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateStructProperty(UScriptStruct *Struct) override
    {
        TSharedPtr<UnLua::ITypeInterface> *PropertyDescPtr = StructPropertyDescMap.Find(Struct);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UStructProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UStructProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Struct);
        return OnPropertyCreated(Property, Struct, StructPropertyDescMap);
    }

    virtual TSharedPtr<UnLua::ITypeInterface> CreateProperty(UProperty *TemplateProperty) override
    {
        // #lizard forgives

        if (!TemplateProperty)
        {
            return nullptr;
        }

        TSharedPtr<UnLua::ITypeInterface> PropertyDesc;
        int32 Type = GetPropertyType(TemplateProperty);
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
            PropertyDesc = CreateIntProperty();
            break;
        case CPT_Float:
        case CPT_Double:
            PropertyDesc = CreateFloatProperty();
            break;
        case CPT_Enum:
            PropertyDesc = CreateEnumProperty(((UEnumProperty*)TemplateProperty)->GetEnum());
            break;
        case CPT_Bool:
            PropertyDesc = CreateBoolProperty();
            break;
        case CPT_ObjectReference:
            {
                UObjectPropertyBase *ObjectBaseProperty = (UObjectPropertyBase*)TemplateProperty;
                if (ObjectBaseProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
                {
                    PropertyDesc = CreateClassProperty(((UClassProperty*)ObjectBaseProperty)->MetaClass);
                }
                else
                {
                    PropertyDesc = CreateObjectProperty(ObjectBaseProperty->PropertyClass);
                }
            }
            break;
        case CPT_WeakObjectReference:
            PropertyDesc = CreateWeakObjectProperty(((UObjectPropertyBase*)TemplateProperty)->PropertyClass);
            break;
        case CPT_LazyObjectReference:
            PropertyDesc = CreateLazyObjectProperty(((UObjectPropertyBase*)TemplateProperty)->PropertyClass);
            break;
        case CPT_SoftObjectReference:
            {
                UObjectPropertyBase *SoftObjectProperty = (USoftObjectProperty*)TemplateProperty;
                if (SoftObjectProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
                {
                    PropertyDesc = CreateSoftClassProperty(((USoftClassProperty*)SoftObjectProperty)->MetaClass);
                }
                else
                {
                    PropertyDesc = CreateSoftObjectProperty(SoftObjectProperty->PropertyClass);
                }
            }
            break;
        case CPT_Interface:
            PropertyDesc = CreateInterfaceProperty(((UInterfaceProperty*)TemplateProperty)->InterfaceClass);
            break;
        case CPT_Name:
            PropertyDesc = CreateNameProperty();
            break;
        case CPT_String:
            PropertyDesc = CreateStringProperty();
            break;
        case CPT_Text:
            PropertyDesc = CreateTextProperty();
            break;
        case CPT_Struct:
            PropertyDesc = CreateStructProperty(((UStructProperty*)TemplateProperty)->Struct);
            break;
        }

        return PropertyDesc;
    }

private:
    /**
     * Cleanup all created properties
     *
     * @param PropertyDescMap - the map the properties reside in
     */
    template <typename KeyType>
    void CleanupProperties(TMap<KeyType*, TSharedPtr<UnLua::ITypeInterface>> &PropertyDescMap)
    {
        for (typename TMap<KeyType*, TSharedPtr<UnLua::ITypeInterface>>::TIterator It(PropertyDescMap); It; ++It)
        {
            Properties.Remove(It.Value()->GetUProperty());
        }
        PropertyDescMap.Empty();
    }

    /**
     * Cleanup non-native properties created before
     *
     * @param PropertyDescMap - the map the properties reside in
     */
    template <typename KeyType>
    void CleanupNonNativeProperties(TMap<KeyType*, TSharedPtr<UnLua::ITypeInterface>> &PropertyDescMap)
    {
        for (typename TMap<KeyType*, TSharedPtr<UnLua::ITypeInterface>>::TIterator It(PropertyDescMap); It; ++It)
        {
            if (!It.Key()->IsNative())
            {
                Properties.Remove(It.Value()->GetUProperty());
                It.RemoveCurrent();
            }
        }
    }

    /**
     * 1. Add the new created UProperty to cluster;
     * 2. Create a 'FPropertyDesc' based on the new created UProperty;
     * 3. Add the new created 'FPropertyDesc' to a map.
     *
     * @param Property - new created UProperty
     * @return - the new created FPropertyDesc
     */
    template <typename KeyType>
    TSharedPtr<UnLua::ITypeInterface> OnPropertyCreated(UProperty *Property, KeyType *Key, TMap<KeyType*, TSharedPtr<UnLua::ITypeInterface>> &PropertyDescMap)
    {
        Property->AddToCluster(ScriptStruct);
        Properties.Add(Property);
        TSharedPtr<UnLua::ITypeInterface> PropertyDesc(FPropertyDesc::Create(Property));
        PropertyDescMap.Add(Key, PropertyDesc);
        return PropertyDesc;
    }

    /**
     * 1. Add the new created UProperty to cluster;
     * 2. Create a 'FPropertyDesc' based on the new created UProperty.
     *
     * @param Property - new created UProperty
     * @return - the new created FPropertyDesc
     */
    TSharedPtr<UnLua::ITypeInterface> OnPropertyCreated(UProperty *Property)
    {
        Property->AddToCluster(ScriptStruct);
        Properties.Add(Property);
        return TSharedPtr<UnLua::ITypeInterface>(FPropertyDesc::Create(Property));
    }

    UScriptStruct *ScriptStruct;
    TSharedPtr<UnLua::ITypeInterface> BoolPropertyDesc;
    TSharedPtr<UnLua::ITypeInterface> IntPropertyDesc;
    TSharedPtr<UnLua::ITypeInterface> FloatPropertyDesc;
    TSharedPtr<UnLua::ITypeInterface> StringPropertyDesc;
    TSharedPtr<UnLua::ITypeInterface> NamePropertyDesc;
    TSharedPtr<UnLua::ITypeInterface> TextPropertyDesc;
    TMap<UEnum*, TSharedPtr<UnLua::ITypeInterface>> EnumPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> ClassPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> ObjectPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> SoftClassPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> SoftObjectPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> WeakObjectPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> LazyObjectPropertyDescMap;
    TMap<UClass*, TSharedPtr<UnLua::ITypeInterface>> InterfacePropertyDescMap;
    TMap<UScriptStruct*, TSharedPtr<UnLua::ITypeInterface>> StructPropertyDescMap;

    TArray<UProperty*> Properties;
};

IPropertyCreator& IPropertyCreator::Instance()
{
    static FPropertyCreator PropertyCreator;
    return PropertyCreator;
}
