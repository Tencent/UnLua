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

#include "UnLuaCompatibility.h"
#include "UnLuaDebugBase.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"
#include "ReflectionUtils/PropertyDesc.h"

namespace UnLua
{

    FString FLuaDebugValue::ToString() const
    {
        FString Description;
        for (int32 i = 0; i < Depth; ++i)
        {
            Description += TEXT("\t");
        }
        Description += ReadableValue;

        for (int32 i = 0; i < Keys.Num(); ++i)
        {
            Description += Keys[i].ToString();
            Description += Values[i].ToString();
            Description += TEXT("\n");
        }

        return Description;
    }

    void FLuaDebugValue::Build(lua_State *L, int32 Index, int32 Level)
    {
        // #lizard forgives

        if (!L || bAlreadyBuilt)
        {
            return;
        }

        int32 ValueType = lua_type(L, Index);
        Type = lua_typename(L, ValueType);
        switch (ValueType)
        {
        case LUA_TNIL:
            ReadableValue = TEXT("nil");
            break;
        case LUA_TBOOLEAN:
            ReadableValue = lua_toboolean(L, Index) > 0 ? TEXT("true") : TEXT("false");
            break;
        case LUA_TLIGHTUSERDATA:
            ReadableValue = FString::Printf(TEXT("light userdata: 0x%p"), lua_topointer(L, Index));
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(L, Index))
            {
                ReadableValue = FString::Printf(TEXT("%d"), lua_tointeger(L, Index));
            }
            else
            {
                ReadableValue = FString::Printf(TEXT("%f"), lua_tonumber(L, Index));
            }
            break;
        case LUA_TSTRING:
            ReadableValue = UTF8_TO_TCHAR(lua_tostring(L, Index));
            break;
        case LUA_TTABLE:
            {
                const void *TableAddress = lua_topointer(L, Index);
                int32 TableSize = 0;
                if (Level > 0)
                {
                    // filter out weak table.
                    if (lua_getmetatable(L, Index))
                    {
                        lua_pushstring(L, "__mode");
                        if( lua_rawget(L, -2) != LUA_TNIL)
                        {
                            ReadableValue = FString::Printf(TEXT("weak table: 0x%p"), TableAddress);
                            lua_pop(L, 2);
                            break;
                        }
                        lua_pop(L, 2);
                    }

                    // traverse all elements
                    Index = (Index < 0 && Index > LUA_REGISTRYINDEX) ? lua_gettop(L) + Index + 1 : Index;
                    lua_pushnil(L);
                    while (lua_next(L, Index) != 0)
                    {
                        FLuaDebugValue *Key = AddKey();
                        if (Key)
                        {
                            Key->Build(L, -2, Level - 1);
                            FLuaDebugValue *Value = AddValue();
                            Value->Build(L, -1, Level - 1);
                        }
                        lua_pop(L, 1);
                        ++TableSize;
                    }
                }
                ReadableValue = FString::Printf(TEXT("table(size=%d): 0x%p"), TableSize, TableAddress);
            }
            break;
        case LUA_TFUNCTION:
            ReadableValue = FString::Printf(TEXT("function: 0x%p"), lua_topointer(L, Index));
            break;
        case LUA_TUSERDATA:
            BuildFromUserdata(L, Index);
            break;
        case LUA_TTHREAD:
            ReadableValue = FString::Printf(TEXT("thread: 0x%p"), lua_topointer(L, Index));
            break;
        }
    }

    FLuaDebugValue* FLuaDebugValue::AddKey()
    {
        // limit value depth
        if (Depth >= MAX_LUA_VALUE_DEPTH)
        {
            return nullptr;
        }

        Keys.AddDefaulted();
        FLuaDebugValue &Key = Keys.Top();
        Key.Depth = Depth + 1;
        return &Key;
    }

    FLuaDebugValue* FLuaDebugValue::AddValue()
    {
        // limit value depth
        if (Depth >= MAX_LUA_VALUE_DEPTH)
        {
            return nullptr;
        }

        Values.AddDefaulted();
        FLuaDebugValue &Value = Values.Top();
        Value.Depth = Depth + 1;
        return &Value;
    }

    /**
     * Get container object based on UStruct
     */
    UObjectBaseUtility* FLuaDebugValue::GetContainerObject(UStruct *Struct, void *ContainerPtr)
    {
        UObjectBaseUtility *ContainerObject = nullptr;
        UClass *Class = Cast<UClass>(Struct);
        if (Class)
        {
            if (Class->IsChildOf(UClass::StaticClass()))
            {
                // UClass
                UClass *MetaClass = ContainerPtr ? (UClass*)ContainerPtr : Class;
                if (MetaClass == UClass::StaticClass())
                {
                    ReadableValue = FString::Printf(TEXT("UClass*: 0x%p"), ContainerPtr);
                }
                else
                {
                    ReadableValue = FString::Printf(TEXT("TSubclassOf<%s%s>: 0x%p"), MetaClass->GetPrefixCPP(), *MetaClass->GetName(), ContainerPtr);
                }
            }
            else
            {
                // UObject
                UObject *Object = (UObject*)ContainerPtr;
                ContainerObject = Object;
                ReadableValue = FString::Printf(TEXT("%s%s* (%s): 0x%p"), Class->GetPrefixCPP(), *Class->GetName(), Object ? *Object->GetName() : TEXT(""), ContainerPtr);
            }
        }
        else
        {
            // UScriptStruct
            UScriptStruct *ScriptStruct = Cast<UScriptStruct>(Struct);
            if (ScriptStruct)
            {
                ReadableValue = FString::Printf(TEXT("F%s: 0x%p"), *ScriptStruct->GetName(), ContainerPtr);
            }
            else
            {
                ReadableValue = FString::Printf(TEXT("userdata (%s): 0x%p"), *FString::Printf(TEXT("%s%s"), Struct->GetPrefixCPP(), *Struct->GetName()), ContainerPtr);
            }
        }

        return ContainerObject;
    }

    /**
     * Build value based on a userdata
     */
    void FLuaDebugValue::BuildFromUserdata(lua_State *L, int32 Index)
    {
        if (lua_type(L, Index) != LUA_TUSERDATA)
        {
            return;
        }

        // get class name
        void *ContainerPtr = UnLua::GetPointer(L, Index);
        const char *ClassNamePtr = nullptr;
        if (lua_getmetatable(L, Index) > 0)
        {
            lua_pushstring(L, "__name");
            lua_rawget(L, -2);
            if (lua_isstring(L, -1))
            {
                ClassNamePtr = lua_tostring(L, -1);
            }
            lua_pop(L, 2);
        }

        if (ClassNamePtr)
        {
            FString ClassName(ClassNamePtr);
            UStruct *Struct = FindFirstObject<UStruct>(*ClassName + 1);
            if (Struct)
            {
                // the userdata is a struct instance
                UObjectBaseUtility *ContainerObject = GetContainerObject(Struct, ContainerPtr);
                BuildFromUStruct(Struct, ContainerPtr, ContainerObject);
            }
            else
            {
                if (!ContainerPtr)
                {
                    ReadableValue = FString::Printf(TEXT("userdata (%s): 0x%p"), *ClassName, ContainerPtr);
                    return;
                }

                if (ClassName == TEXT("TArray"))
                {
                    // the userdata is a TArray instance
                    FLuaArray *Array = (FLuaArray*)ContainerPtr;
                    FProperty *InnerProperty = Array->Inner->GetUProperty();
                    if (InnerProperty)
                    {
                        FScriptArrayHelper ArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(InnerProperty, Array->ScriptArray);
                        BuildFromTArray(ArrayHelper, InnerProperty);
                    }
                    else
                    {
                        ReadableValue = FString::Printf(TEXT("TArray<UnknownType> Num=%d, Not Support Expand"), Array->Num());
                    }
                }
                else if (ClassName == TEXT("TMap"))
                {
                    // the userdata is a TMap instance
                    FLuaMap *Map = (FLuaMap*)ContainerPtr;
                    FProperty *KeyProperty = Map->KeyInterface->GetUProperty();
                    FProperty *ValueProperty = Map->ValueInterface->GetUProperty();
                    if (KeyProperty && ValueProperty)
                    {
                        FScriptMapHelper MapHelper = FScriptMapHelper::CreateHelperFormInnerProperties(KeyProperty, ValueProperty, Map->Map);
                        BuildFromTMap(MapHelper);
                    }
                    else
                    {
                        ReadableValue = FString::Printf(TEXT("TMap<UnknownType, UnknownType> Num=%d, Not Support Expand"), Map->Num());
                    }
                }
                else if (ClassName == TEXT("TSet"))
                {
                    // the userdata is a TSet instance
                    FLuaSet *Set = (FLuaSet*)ContainerPtr;
                    FProperty *ElementProperty = Set->ElementInterface->GetUProperty();
                    if (ElementProperty)
                    {
                        FScriptSetHelper SetHelper = FScriptSetHelper::CreateHelperFormElementProperty(ElementProperty, Set->Set);
                        BuildFromTSet(SetHelper);
                    }
                    else
                    {
                        ReadableValue = FString::Printf(TEXT("TSet<UnknownType> Num=%d, Not Support Expand"), Set->Num());
                    }
                }
                else if (ClassName == TEXT("FSoftObjectPtr"))
                {
                    // the userdata is a FSoftObjectPtr instance
                    FSoftObjectPtr *SoftObject = (FSoftObjectPtr*)ContainerPtr;
                    ReadableValue = FString::Printf(TEXT("TSoftObjectPtr<UObject>: %s 0x%p"), *SoftObject->ToString(), SoftObject->Get());
                }
                else
                {
                    ReadableValue = FString::Printf(TEXT("userdata (%s): 0x%p"), *ClassName, ContainerPtr);
                }
            }
        }
        else
        {
            ReadableValue = FString::Printf(TEXT("userdata: 0x%p"), ContainerPtr);
        }
    }

    /**
     * Build value based on a UStruct
     */
    void FLuaDebugValue::BuildFromUStruct(UStruct *Struct, void *ContainerPtr, UObjectBaseUtility *ContainerObject)
    {
        if (!Struct || !ContainerPtr)
        {
            return;
        }

        if (ContainerObject && ContainerObject->HasAnyFlags(RF_NeedInitialization))
        {
            return;
        }

        for (FProperty *Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
        {
            // filter out deprecated FProperty
            if (Property->HasAnyPropertyFlags(CPF_Deprecated))
            {
                continue;
            }

            FLuaDebugValue *Key = AddKey();
            if (Key)
            {
                Key->ReadableValue = Property->GetNameCPP();
            }
            FLuaDebugValue *Value = AddValue();
            if (Value)
            {
                Value->BuildFromUProperty(Property, Property->ContainerPtrToValuePtr<void>(ContainerPtr));
            }
        }
    }

    /**
     * Build value based on a TArray
     */
    void FLuaDebugValue::BuildFromTArray(FScriptArrayHelper &ArrayHelper, const FProperty *InnerProperty)
    {
        check(InnerProperty);
        FString InnerExtendedTypeText;
        FString InnerTypeText = InnerProperty->GetCPPType(&InnerExtendedTypeText);
        ReadableValue = FString::Printf(TEXT("TArray<%s%s> Num=%d"), *InnerTypeText, *InnerExtendedTypeText, ArrayHelper.Num());
        for (int32 i = 0; i < ArrayHelper.Num(); ++i)
        {
            FLuaDebugValue *Key = AddKey();
            if (Key)
            {
                Key->ReadableValue = FString::Printf(TEXT("[%d]"), i + 1);
            }
            FLuaDebugValue *Value = AddValue();
            if (Value)
            {
                Value->BuildFromUProperty(InnerProperty, ArrayHelper.GetRawPtr(i));
            }
        }
    }

    /**
     * Build value based on a TMap
     */
    void FLuaDebugValue::BuildFromTMap(FScriptMapHelper &MapHelper)
    {
        FString KeyExtendedTypeText, ValueExtendedTypeText;
        FString KeyTypeText = MapHelper.KeyProp->GetCPPType(&KeyExtendedTypeText);
        FString ValueTypeText = MapHelper.ValueProp->GetCPPType(&ValueExtendedTypeText);
        ReadableValue = FString::Printf(TEXT("TMap<%s%s,%s%s> Num=%d"), *KeyTypeText, *KeyExtendedTypeText, *ValueTypeText, *ValueExtendedTypeText, MapHelper.Num());
        int32 i = -1, Size = MapHelper.Num();
        while (Size > 0)
        {
            if (MapHelper.IsValidIndex(++i))
            {
                FLuaDebugValue *Key = AddKey();
                if (Key)
                {
                    Key->BuildFromUProperty(MapHelper.KeyProp, MapHelper.GetKeyPtr(i));
                }
                FLuaDebugValue *Value = AddValue();
                if (Value)
                {
                    Value->BuildFromUProperty(MapHelper.ValueProp, MapHelper.GetValuePtr(i));
                }
                --Size;
            }
        }
    }

    /**
     * Build value based on a TSet
     */
    void FLuaDebugValue::BuildFromTSet(FScriptSetHelper &SetHelper)
    {
        FString ElementExtendedTypeText;
        FString ElementTypeText = SetHelper.ElementProp->GetCPPType(&ElementExtendedTypeText);
        ReadableValue = FString::Printf(TEXT("TSet<%s%s> Num=%d"), *ElementTypeText, *ElementExtendedTypeText, SetHelper.Num());
        int32 i = -1, j = 1, Size = SetHelper.Num();
        while (Size > 0)
        {
            if (SetHelper.IsValidIndex(++i))
            {
                FLuaDebugValue *Key = AddKey();
                if (Key)
                {
                    Key->ReadableValue = FString::Printf(TEXT("[%d]"), j++);
                }
                FLuaDebugValue *Value = AddValue();
                if (Value)
                {
                    Value->BuildFromUProperty(SetHelper.ElementProp, SetHelper.GetElementPtr(i));
                }
                --Size;
            }
        }
    }

    /**
     * Build value based on a FProperty
     */
    void FLuaDebugValue::BuildFromUProperty(const FProperty *InProperty, void *ValuePtr)
    {
        // #lizard forgives

        if (!InProperty || !ValuePtr)
        {
            return;
        }

        int32 PropertyType = GetPropertyType(InProperty);
        switch (PropertyType)
        {
        case CPT_Int8:
        case CPT_Int16:
        case CPT_Int:
        case CPT_Int64:
        case CPT_Byte:
        case CPT_UInt16:
        case CPT_UInt32:
        case CPT_UInt64:
            {
                FNumericProperty *NumericProperty = (FNumericProperty*)InProperty;
                ReadableValue = FString::Printf(TEXT("%ld"), NumericProperty->GetUnsignedIntPropertyValue(ValuePtr));
            }
            break;
        case CPT_Float:
        case CPT_Double:
            {
                FNumericProperty *NumericProperty = (FNumericProperty*)InProperty;
                ReadableValue = FString::Printf(TEXT("%lf"), NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
            }
            break;
        case CPT_Enum:
            {
                FEnumProperty *EnumProperty = (FEnumProperty*)InProperty;
                int64 Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
                ReadableValue = FString::Printf(TEXT("%s: %s"), *EnumProperty->GetCPPType(nullptr, 0), *EnumProperty->GetEnum()->GetNameStringByValue(Value));
            }
            break;
        case CPT_Bool:
            {
                FBoolProperty *BoolProperty = (FBoolProperty*)InProperty;
                ReadableValue = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false");
            }
            break;
        case CPT_ObjectReference:
            {
                FObjectProperty *ObjectProperty = (FObjectProperty*)InProperty;
                //UObject *Object = ObjectProperty->GetPropertyValue(ValuePtr);
                //LuaValue->ReadableValue = FString::Printf(TEXT("%s: %p"), *ObjectProperty->GetCPPType(nullptr, 0), Object);
                //if (Object && !ObjectProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
                //{
                //    GetUStructValue(Object->GetClass(), Object, LuaValue);
                //}
                if (ObjectProperty->PropertyClass->IsChildOf(UClass::StaticClass()))
                {
                    // UClass
                    FClassProperty *ClassProperty = (FClassProperty*)ObjectProperty;
                    UClass *Class = Cast<UClass>(ObjectProperty->GetPropertyValue(ValuePtr));
#if UE_VERSION_OLDER_THAN(5, 3, 0)
                    UClass *MetaClass = Class ? Class : ClassProperty->MetaClass;
#else
                    UClass *MetaClass = Class ? Class : ClassProperty->MetaClass.Get();
#endif
                    if (MetaClass == UClass::StaticClass())
                    {
                        ReadableValue = FString::Printf(TEXT("UClass*: 0x%p"), Class);
                    }
                    else
                    {
                        ReadableValue = FString::Printf(TEXT("TSubclassOf<%s%s>: 0x%p"), MetaClass->GetPrefixCPP(), *MetaClass->GetName(), Class);
                    }
                }
                else
                {
                    // UObject
                    UObject *Object = ObjectProperty->GetPropertyValue(ValuePtr);
#if UE_VERSION_OLDER_THAN(5, 3, 0)
                    UClass *Class = Object ? Object->GetClass() : ObjectProperty->PropertyClass;
#else
                    UClass *Class = Object ? Object->GetClass() : ObjectProperty->PropertyClass.Get();
#endif
                    ReadableValue = FString::Printf(TEXT("%s%s* (%s): 0x%p"), Class->GetPrefixCPP(), *Class->GetName(), Object ? *Object->GetName() : TEXT(""), Object);
                    BuildFromUStruct(Class, Object, Object);
                }
            }
            break;
        case CPT_WeakObjectReference:    // serial number and index (in 'GUObjectArray') of the object
            {
                FWeakObjectProperty *WeakObjectProperty = (FWeakObjectProperty*)InProperty;
                const FWeakObjectPtr &WeakObject = WeakObjectProperty->GetPropertyValue(ValuePtr);
                UObject *Object = WeakObject.Get();
                UClass *Class = Object ? Object->GetClass() : nullptr;
                ReadableValue = FString::Printf(TEXT("%s: 0x%p"), *WeakObjectProperty->GetCPPType(nullptr, 0), Object);
                BuildFromUStruct(Class, Object, Object);
            }
            break;
        case CPT_LazyObjectReference:    // GUID of the object
            {
                FLazyObjectProperty *LazyObjectProperty = (FLazyObjectProperty*)InProperty;
                const FLazyObjectPtr &LazyObject = LazyObjectProperty->GetPropertyValue(ValuePtr);
                UObject *Object = LazyObject.Get();
                UClass *Class = Object ? Object->GetClass() : nullptr;
                ReadableValue = FString::Printf(TEXT("%s: %s 0x%p"), *LazyObjectProperty->GetCPPType(nullptr, 0), *LazyObject.GetUniqueID().ToString(), Object);
                BuildFromUStruct(Class, Object, Object);
            }
            break;
        case CPT_SoftObjectReference:    // path of the object
            {
                FSoftObjectProperty *SoftObjectProperty = (FSoftObjectProperty*)InProperty;
                const FSoftObjectPtr &SoftObject = SoftObjectProperty->GetPropertyValue(ValuePtr);
                UObject *Object = SoftObject.Get();
                UClass *Class = Object ? Object->GetClass() : nullptr;
                ReadableValue = FString::Printf(TEXT("%s: %s 0x%p"), *SoftObjectProperty->GetCPPType(nullptr, 0), *SoftObject.ToString(), Object);
                BuildFromUStruct(Class, Object, Object);
            }
            break;
        case CPT_Interface:
            {
                FInterfaceProperty *InterfaceProperty = (FInterfaceProperty*)InProperty;
                FString InterfaceExtendedTypeText;
                FString InterfaceTypeText = InterfaceProperty->GetCPPType(&InterfaceExtendedTypeText, 0);
                const FScriptInterface &Interface = InterfaceProperty->GetPropertyValue(ValuePtr);
                UObject *Object = Interface.GetObject();
                UClass *Class = Object ? Object->GetClass() : nullptr;
                ReadableValue = FString::Printf(TEXT("%s%s: Object(0x%p), Interface(0x%p)"), *InterfaceTypeText, *InterfaceExtendedTypeText, Object, Interface.GetInterface());
                BuildFromUStruct(Class, Object, Object);
            }
            break;
        case CPT_Name:
            {
                FNameProperty *NameProperty = (FNameProperty*)InProperty;
                ReadableValue = NameProperty->GetPropertyValue(ValuePtr).ToString();
            }
            break;
        case CPT_String:
            {
                FStrProperty *StringProperty = (FStrProperty*)InProperty;
                ReadableValue = StringProperty->GetPropertyValue(ValuePtr);
            }
            break;
        case CPT_Text:
            {
                FTextProperty *TextProperty = (FTextProperty*)InProperty;
                ReadableValue = TextProperty->GetPropertyValue(ValuePtr).ToString();
            }
            break;
        case CPT_Array:
            {
                FArrayProperty *ArrayProperty = (FArrayProperty*)InProperty;
                FScriptArray *ScriptArray = (FScriptArray*)(&ArrayProperty->GetPropertyValue(ValuePtr));
                FScriptArrayHelper ArrayHelper(ArrayProperty, ScriptArray);
                BuildFromTArray(ArrayHelper, ArrayProperty->Inner);
            }
            break;
        case CPT_Map:
            {
                FMapProperty *MapProperty = (FMapProperty*)InProperty;
                FScriptMap *ScriptMap = (FScriptMap*)(&MapProperty->GetPropertyValue(ValuePtr));
                FScriptMapHelper MapHelper(MapProperty, ScriptMap);
                BuildFromTMap(MapHelper);
            }
            break;
        case CPT_Set:
            {
                FSetProperty *SetProperty = (FSetProperty*)InProperty;
                FScriptSet *ScriptSet = (FScriptSet*)(&SetProperty->GetPropertyValue(ValuePtr));
                FScriptSetHelper SetHelper(SetProperty, ScriptSet);
                BuildFromTSet(SetHelper);
            }
            break;
        case CPT_Struct:
            {
                FStructProperty *StructProperty = (FStructProperty*)InProperty;
                ReadableValue = FString::Printf(TEXT("%s: 0x%p"), *StructProperty->GetCPPType(nullptr, 0), ValuePtr);
                BuildFromUStruct(StructProperty->Struct, ValuePtr);
            }
            break;
        default:
            ReadableValue = FString::Printf(TEXT("Not Support Expand Type : %d"), PropertyType);
            break;
        }
    }


    bool GetStackVariables(lua_State *L, int32 StackLevel, TArray<FLuaVariable> &LocalVariables, TArray<FLuaVariable> &Upvalues, int32 Level)
    {
        if (!L)
        {
            return false;
        }
        lua_Debug ar;
        if (!lua_getstack(L, StackLevel, &ar))
        {
            return false;
        }
        if (!lua_getinfo(L, "nSlu", &ar))
        {
            return false;
        }

        for (int32 i = 1; ; ++i)
        {
            const char *VarName = lua_getlocal(L, &ar, i);
            if (!VarName)
            {
                break;
            }
            if (VarName[0] == '(')
            {
                lua_pop(L, 1);
                continue;
            }
            LocalVariables.AddDefaulted();
            FLuaVariable &Variable = LocalVariables.Top();
            Variable.Key = UTF8_TO_TCHAR(VarName);
            Variable.Value.Depth = 0;
            Variable.Value.Build(L, -1, Level);
            lua_pop(L, 1);
        }

        if (lua_getinfo(L, "f", &ar))
        {
            int32 FunctionIdx = lua_gettop(L);
            for (int32 i = 1; ; ++i)
            {
                const char *UpvalueName = lua_getupvalue(L, FunctionIdx, i);
                if (!UpvalueName)
                {
                    break;
                }
                Upvalues.AddDefaulted();
                FLuaVariable &Upvalue = Upvalues.Top();
                Upvalue.Key = UTF8_TO_TCHAR(UpvalueName);
                Upvalue.Value.Depth = 0;
                Upvalue.Value.Build(L, -1, Level);
                lua_pop(L, 1);
            }
        }

        return true;
    }

    FString GetLuaCallStack(lua_State *L)
    {
        if (!L)
        {
            return FString();
        }

        int32 Depth = 0;
        lua_Debug ar;
        FString CallStack = TEXT("Lua stack : \n");
        while (lua_getstack(L, Depth++, &ar))
        {
            lua_getinfo(L, "nSl", &ar);
            FString DisplayInfo = FString::Printf(TEXT("Source : %s, name : %s, Line : %d \n"), UTF8_TO_TCHAR(ar.source), UTF8_TO_TCHAR(ar.name), ar.currentline);
            CallStack += DisplayInfo;
        }

        //if (!bIncludeVariables)
        //{
        //    return CallStack;
        //}

        //TArray<UnLua::FLuaVariable> LocalVariables;
        //TArray<UnLua::FLuaVariable> Upvalues;
        //if (UnLua::GetStackVariables(L, 1, LocalVariables, Upvalues))     // get variable infos of level 1
        //{
        //    CallStack += TEXT("\nLocalValue :");
        //    for (const UnLua::FLuaVariable &Var : LocalVariables)
        //    {
        //        if (Var.Key.Find(TEXT("_ENV")) >= 0)
        //        {
        //            continue;
        //        }
        //        FString DisplayInfo = FString::Printf(TEXT(" %s : %s \n"), *Var.Key, *Var.Value.ToString());
        //        CallStack += DisplayInfo;
        //    }

        //    CallStack += TEXT("\nUpValue :");
        //    for (const UnLua::FLuaVariable &Var : Upvalues)
        //    {
        //        if (Var.Key.Find(TEXT("_ENV")) >= 0)
        //        {
        //            continue;
        //        }
        //        FString DisplayInfo = FString::Printf(TEXT(" %s : %s \n"), *Var.Key, *Var.Value.ToString());
        //        CallStack += DisplayInfo;
        //    }
        //}

        return CallStack;
    }

    void PrintCallStack(lua_State* L)
    {
        if (!L)
            return;

        luaL_traceback(L, L, "", 0);
        UE_LOG(LogUnLua, Log, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(L,-1)));
        lua_pop(L, 1);
    }
}
