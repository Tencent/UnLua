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
        : ScriptStruct(nullptr), BoolPropertyDesc(nullptr), IntPropertyDesc(nullptr), FloatPropertyDesc(nullptr), StringPropertyDesc(nullptr), NamePropertyDesc(nullptr), TextPropertyDesc(nullptr)
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
        FPropertyDesc::Release(BoolPropertyDesc);
        FPropertyDesc::Release(IntPropertyDesc);
        FPropertyDesc::Release(FloatPropertyDesc);
        FPropertyDesc::Release(StringPropertyDesc);
        FPropertyDesc::Release(NamePropertyDesc);
        FPropertyDesc::Release(TextPropertyDesc);

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

    virtual FPropertyDesc* CreateBoolProperty() override
    {
        if (!BoolPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UBoolProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UBoolProperty(FObjectInitializer(), EC_CppProperty, 0, (EPropertyFlags)0, 0xFF, 1, true);
            BoolPropertyDesc = OnPropertyCreated(Property);
        }
        return BoolPropertyDesc;
    }

    virtual FPropertyDesc* CreateIntProperty() override
    {
        if (!IntPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UIntProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UIntProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            IntPropertyDesc = OnPropertyCreated(Property);
        }
        return IntPropertyDesc;
    }

    virtual FPropertyDesc* CreateFloatProperty() override
    {
        if (!FloatPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UFloatProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UFloatProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            FloatPropertyDesc = OnPropertyCreated(Property);
        }
        return FloatPropertyDesc;
    }

    virtual FPropertyDesc* CreateStringProperty() override
    {
        if (!StringPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UStrProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UStrProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            StringPropertyDesc = OnPropertyCreated(Property);
        }
        return StringPropertyDesc;
    }

    virtual FPropertyDesc* CreateNameProperty() override
    {
        if (!NamePropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UNameProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UNameProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            NamePropertyDesc = OnPropertyCreated(Property);
        }
        return NamePropertyDesc;
    }

    virtual FPropertyDesc* CreateTextProperty() override
    {
        if (!TextPropertyDesc)
        {
            // see overloaded operator new that defined in DECLARE_CLASS(...)
            UTextProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UTextProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash);
            TextPropertyDesc = OnPropertyCreated(Property);
        }
        return TextPropertyDesc;
    }

    virtual FPropertyDesc* CreateEnumProperty(UEnum *Enum) override
    {
        FPropertyDesc **PropertyDescPtr = EnumPropertyDescMap.Find(Enum);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UEnumProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UEnumProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Enum);
        UNumericProperty *UnderlyingProp = NewObject<UByteProperty>(Property, TEXT("UnderlyingType"));
        Property->AddCppProperty(UnderlyingProp);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Enum, EnumPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateClassProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = ClassPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSubclassOf<...>
        UClassProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UClassProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class, nullptr);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, ClassPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateObjectProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = ObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, ObjectPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateSoftClassProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = SoftClassPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSoftClassPtr<...>
        USoftClassProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) USoftClassProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, SoftClassPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateSoftObjectProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = SoftObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TSoftObjectPtr<...>
        USoftObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) USoftObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, SoftObjectPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateWeakObjectProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = WeakObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TWeakObjectPtr<...>
        UWeakObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UWeakObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, WeakObjectPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateLazyObjectProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = LazyObjectPropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TLazyObjectPtr<...>
        ULazyObjectProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) ULazyObjectProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, LazyObjectPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateInterfaceProperty(UClass *Class) override
    {
        FPropertyDesc **PropertyDescPtr = InterfacePropertyDescMap.Find(Class);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...). TScriptInterface<...>
        UInterfaceProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UInterfaceProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash | CPF_UObjectWrapper, Class);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Class, InterfacePropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateStructProperty(UScriptStruct *Struct) override
    {
        FPropertyDesc **PropertyDescPtr = StructPropertyDescMap.Find(Struct);
        if (PropertyDescPtr)
        {
            return *PropertyDescPtr;
        }

        // see overloaded operator new that defined in DECLARE_CLASS(...)
        UStructProperty *Property = new (EC_InternalUseOnlyConstructor, ScriptStruct, NAME_None, RF_Transient) UStructProperty(FObjectInitializer(), EC_CppProperty, 0, CPF_HasGetValueTypeHash, Struct);
        FPropertyDesc *PropertyDesc = OnPropertyCreated(Property, Struct, StructPropertyDescMap);
        return PropertyDesc;
    }

    virtual FPropertyDesc* CreateProperty(UProperty *TemplateProperty) override
    {
        // #lizard forgives

        if (!TemplateProperty)
        {
            return nullptr;
        }

        FPropertyDesc *PropertyDesc = nullptr;
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
    void CleanupProperties(TMap<KeyType*, FPropertyDesc*> &PropertyDescMap)
    {
        for (typename TMap<KeyType*, FPropertyDesc*>::TIterator It(PropertyDescMap); It; ++It)
        {
            FPropertyDesc *PropertyDesc = It.Value();
            Properties.Remove(PropertyDesc->GetProperty());
            FPropertyDesc::Release(PropertyDesc);
        }
        PropertyDescMap.Empty();
    }

    /**
     * Cleanup non-native properties created before
     *
     * @param PropertyDescMap - the map the properties reside in
     */
    template <typename KeyType>
    void CleanupNonNativeProperties(TMap<KeyType*, FPropertyDesc*> &PropertyDescMap)
    {
        for (typename TMap<KeyType*, FPropertyDesc*>::TIterator It(PropertyDescMap); It; ++It)
        {
            if (!It.Key()->IsNative())
            {
                FPropertyDesc *PropertyDesc = It.Value();
                Properties.Remove(PropertyDesc->GetProperty());
                FPropertyDesc::Release(PropertyDesc);
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
    FPropertyDesc* OnPropertyCreated(UProperty *Property, KeyType *Key, TMap<KeyType*, FPropertyDesc*> &PropertyDescMap)
    {
        Property->AddToCluster(ScriptStruct);
        Properties.Add(Property);
        FPropertyDesc *PropertyDesc = FPropertyDesc::Create(Property);
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
    FPropertyDesc* OnPropertyCreated(UProperty *Property)
    {
        Property->AddToCluster(ScriptStruct);
        Properties.Add(Property);
        return FPropertyDesc::Create(Property);
    }

    UScriptStruct *ScriptStruct;
    FPropertyDesc *BoolPropertyDesc;
    FPropertyDesc *IntPropertyDesc;
    FPropertyDesc *FloatPropertyDesc;
    FPropertyDesc *StringPropertyDesc;
    FPropertyDesc *NamePropertyDesc;
    FPropertyDesc *TextPropertyDesc;
    TMap<UEnum*, FPropertyDesc*> EnumPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> ClassPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> ObjectPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> SoftClassPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> SoftObjectPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> WeakObjectPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> LazyObjectPropertyDescMap;
    TMap<UClass*, FPropertyDesc*> InterfacePropertyDescMap;
    TMap<UScriptStruct*, FPropertyDesc*> StructPropertyDescMap;

    TArray<UProperty*> Properties;
};

IPropertyCreator& IPropertyCreator::Instance()
{
    static FPropertyCreator PropertyCreator;
    return PropertyCreator;
}
