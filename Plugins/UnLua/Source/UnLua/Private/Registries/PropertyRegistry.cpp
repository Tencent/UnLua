#include "PropertyRegistry.h"
#include "Binding.h"
#include "ClassRegistry.h"
#include "EnumRegistry.h"
#include "LowLevel.h"
#include "ReflectionUtils/PropertyDesc.h"

namespace UnLua
{
    FPropertyRegistry::FPropertyRegistry(FLuaEnv* Env)
        : Env(Env)
    {
        PropertyCollector = FindObject<UScriptStruct>(ANY_PACKAGE, TEXT("PropertyCollector"));
        check(PropertyCollector);
    }

    void FPropertyRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        FieldProperties.Remove(static_cast<UField*>(Object));
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::CreateTypeInterface(lua_State* L, int32 Index)
    {
        Index = LowLevel::AbsIndex(L, Index);

        TSharedPtr<ITypeInterface> TypeInterface;
        int32 Type = lua_type(L, Index);
        switch (Type)
        {
        case LUA_TBOOLEAN:
            TypeInterface = GetBoolProperty();
            break;
        case LUA_TNUMBER:
            TypeInterface = lua_isinteger(L, Index) > 0 ? GetIntProperty() : GetFloatProperty();
            break;
        case LUA_TSTRING:
            TypeInterface = GetStringProperty();
            break;
        case LUA_TTABLE:
            {
                lua_pushstring(L, "__name");
                Type = lua_rawget(L, Index);
                if (Type == LUA_TSTRING)
                {
                    const char* Name = lua_tostring(L, -1);
                    auto ClassDesc = FClassRegistry::Find(Name);
                    if (ClassDesc)
                    {
                        TypeInterface = GetFieldProperty(ClassDesc->AsStruct());
                    }
                    else
                    {
                        auto EnumDesc = FEnumRegistry::Find(Name);
                        if (EnumDesc)
                            TypeInterface = GetFieldProperty(EnumDesc->GetEnum());
                        else
                            TypeInterface = FindTypeInterface(lua_tostring(L, -1));
                    }
                }
                lua_pop(L, 1);
            }
            break;
        case LUA_TUSERDATA:
            {
                // mt/nil
                lua_getmetatable(L, Index);
                if (lua_istable(L, -1))
                {
                    // mt,mt.__name/nil
                    lua_getfield(L, -1, "__name");
                    if (lua_isstring(L, -1))
                    {
                        const char* Name = lua_tostring(L, -1);
                        FClassDesc* ClassDesc = FClassRegistry::Find(Name);
                        if (ClassDesc)
                            TypeInterface = GetFieldProperty(ClassDesc->AsStruct());
                    }
                    // mt
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
            break;
        default:
            check(false);
        }

        return TypeInterface;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetBoolProperty()
    {
        if (!BoolProperty)
        {
            const auto Property = new FBoolProperty(PropertyCollector, NAME_None, RF_Transient, 0, (EPropertyFlags)0, 0xFF, 1, true);
            BoolProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return BoolProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetIntProperty()
    {
        if (!IntProperty)
        {
            const auto Property = new FIntProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash);
            IntProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return IntProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetFloatProperty()
    {
        if (!FloatProperty)
        {
            const auto Property = new FFloatProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash);
            FloatProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return FloatProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetStringProperty()
    {
        if (!StringProperty)
        {
            const auto Property = new FStrProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash);
            StringProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return StringProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetNameProperty()
    {
        if (!NameProperty)
        {
            const auto Property = new FNameProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash);
            NameProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return NameProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetTextProperty()
    {
        if (!TextProperty)
        {
            const auto Property = new FTextProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash);
            TextProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return TextProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetFieldProperty(UField* Field)
    {
        const auto Exists = FieldProperties.Find(Field);
        if (Exists)
            return *Exists;

        FProperty* Property;
        if (const auto Class = Cast<UClass>(Field))
        {
            Property = new FObjectProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash, Class);
        }
        else if (const auto ScriptStruct = Cast<UScriptStruct>(Field))
        {
            Property = new FStructProperty(PropertyCollector, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash, ScriptStruct);
        }
        else if (const auto Enum = Cast<UEnum>(Field))
        {
            const auto EnumProperty = new FEnumProperty(ScriptStruct, NAME_None, RF_Transient, 0, CPF_HasGetValueTypeHash, Enum);
            const auto UnderlyingProperty = new FByteProperty(EnumProperty, TEXT("UnderlyingType"), RF_Transient);
            Property = EnumProperty;
            Property->AddCppProperty(UnderlyingProperty);
            Property->ElementSize = UnderlyingProperty->ElementSize;
            Property->PropertyFlags |= CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor;
        }
        else
        {
            Property = nullptr;
        }

        const auto Ret = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        FieldProperties.Add(Field, Ret);
        return Ret;
    }
}
