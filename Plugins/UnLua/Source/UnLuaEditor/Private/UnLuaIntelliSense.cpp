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

#include "UnLuaIntelliSense.h"

#include "ObjectEditorUtils.h"
#include "UnLuaInterface.h"
#include "UObject/MetaData.h"

static const FName NAME_ToolTip(TEXT("ToolTip")); // key of ToolTip meta data
static const FName NAME_LatentInfo = TEXT("LatentInfo"); // tag of latent function

FString UnLua::IntelliSense::Get(const UBlueprint* Blueprint)
{
    FString BPName = Blueprint->GetName();
    FString Ret = Get(Blueprint->GeneratedClass);
    Ret += "local M = {}\r\n\r\n";

    const UClass* GeneratedClass = *Blueprint->GeneratedClass;

    // functions
    for (TFieldIterator<UFunction> FunctionIt(GeneratedClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FunctionIt; ++FunctionIt)
    {
        const UFunction* Function = *FunctionIt;
        if (!IsValid(Function))
            continue;
        if (FObjectEditorUtils::IsFunctionHiddenFromClass(Function, GeneratedClass))
            continue;
        Ret += Get(Function) + "\r\n";
    }

    // interfaces
    for (int32 i = 0; i < Blueprint->ImplementedInterfaces.Num(); i++)
    {
        const FBPInterfaceDescription& InterfaceDesc = Blueprint->ImplementedInterfaces[i];
        const UClass* InterfaceClass = InterfaceDesc.Interface.Get();
        if (!InterfaceClass || InterfaceClass == UUnLuaInterface::StaticClass())
            continue;

        for (TFieldIterator<UFunction> FunctionIt(InterfaceClass, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
        {
            const UFunction* Function = *FunctionIt;
            if (!IsValid(Function))
                continue;
            Ret += Get(Function) + "\r\n";
        }
    }

    Ret += "return M";
    return Ret;
}

FString UnLua::IntelliSense::Get(const UStruct* Struct)
{
    if (!Struct)
        return "";

    FString Ret = "---@class " + GetTypeName(Struct);
    UStruct* SuperStruct = Struct->GetSuperStruct();
    if (SuperStruct)
        Ret += " : " + GetTypeName(SuperStruct);
    Ret += "\r\n";

    for (TFieldIterator<FProperty> It(Struct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); It; ++It)
    {
        const FProperty* Property = *It;
        Ret += Get(Property) += "\r\n";
    }

    return Ret;
}

FString UnLua::IntelliSense::Get(const UFunction* Function)
{
    FString Ret;
    FString Properties;
    TMap<FName, FString>* MetaMap = UMetaData::GetMapForObject(Function);

    if (MetaMap)
    {
        const FString* ToolTip = MetaMap->Find(NAME_ToolTip);
        if (ToolTip && !ToolTip->IsEmpty())
        {
            Ret += "---" + ToolTip->Replace(TEXT("\n"), TEXT("\r\n---"));
            Ret += "\r\n";
        }
    }
    
    // black list of Lua key words
    static FString LuaKeyWords[] = {TEXT("local"), TEXT("function"), TEXT("end")};
    static constexpr int32 NumLuaKeyWords = sizeof(LuaKeyWords) / sizeof(FString);

    for (TFieldIterator<FProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
    {
        const FProperty* Property = *It;
        if (Property->GetFName() == NAME_LatentInfo)
            continue;

        FString TypeName = GetTypeName(Property);
        const FString& PropertyComment = Property->GetMetaData(NAME_ToolTip);
        FString ExtraDesc;

        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            Ret += FString::Printf(TEXT("---@return %s"), *TypeName); // return parameter
        }
        else
        {
            FString PropertyName = Property->GetName();
            for (int32 KeyWordIdx = 0; KeyWordIdx < NumLuaKeyWords; ++KeyWordIdx)
            {
                if (PropertyName == LuaKeyWords[KeyWordIdx])
                {
                    PropertyName += TEXT("__"); // add suffix for Lua key words
                    break;
                }
            }

            if (Properties.IsEmpty())
                Properties = PropertyName;
            else
                Properties += FString::Printf(TEXT(", %s"), *PropertyName);

            if (MetaMap)
            {
                FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
                FString* ValuePtr = MetaMap->Find(KeyName); // find default parameter value
                if (ValuePtr)
                {
                    ExtraDesc = TEXT("[opt]"); // default parameter
                }
                else if (Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
                {
                    ExtraDesc = TEXT("[out]"); // non-const reference
                }
            }
            Ret += FString::Printf(TEXT("---@param %s %s"), *PropertyName, *TypeName);
        }

        if (ExtraDesc.Len() > 0 || PropertyComment.Len() > 0)
        {
            Ret += TEXT(" @");
            if (ExtraDesc.Len() > 0)
                Ret += FString::Printf(TEXT("%s "), *ExtraDesc);
        }

        Ret += TEXT("\r\n");
    }

    Ret += FString::Printf(TEXT("function M%s%s(%s) end\r\n\r\n"), Function->HasAnyFunctionFlags(FUNC_Static) ? TEXT(".") : TEXT(":"), *Function->GetName(), *Properties);
    return Ret;
}

FString UnLua::IntelliSense::Get(const FProperty* Property)
{
    FString Ret;

    const UStruct* Struct = Property->GetOwnerStruct();

    // access level
    FString AccessLevel;
    if (Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
        AccessLevel = TEXT("public");
    else if (Property->HasAllPropertyFlags(CPF_NativeAccessSpecifierProtected))
        AccessLevel = TEXT("protected");
    else if (Property->HasAllPropertyFlags(CPF_NativeAccessSpecifierPrivate))
        AccessLevel = TEXT("private");
    else
        AccessLevel = Struct->IsNative() ? "private" : "public";

    FString TypeName = IntelliSense::GetTypeName(Property);
    Ret += FString::Printf(TEXT("---@field %s %s %s"), *AccessLevel, *Property->GetName(), *TypeName);

    // comment
    const FString& ToolTip = Property->GetMetaData(NAME_ToolTip);
    if (!ToolTip.IsEmpty())
        Ret += " @" + EscapeComments(ToolTip);

    return Ret;
}

FString UnLua::IntelliSense::GetTypeName(const UObject* Field)
{
    if (!Field)
        return "";
    if (!Field->IsNative())
        return Field->GetName().LeftChop(2);
    const UStruct* Struct = Cast<UStruct>(Field);
    if (Struct)
        return Struct->GetPrefixCPP() + Struct->GetName();
    return Field->GetName();
}

FString UnLua::IntelliSense::GetTypeName(const FProperty* Property)
{
    if (!Property)
        return "Unknown";

    if (CastField<FByteProperty>(Property))
        return "integer";

    if (CastField<FInt8Property>(Property))
        return "integer";

    if (CastField<FInt16Property>(Property))
        return "integer";

    if (CastField<FIntProperty>(Property))
        return "integer";

    if (CastField<FInt64Property>(Property))
        return "integer";

    if (CastField<FUInt16Property>(Property))
        return "integer";

    if (CastField<FUInt32Property>(Property))
        return "integer";

    if (CastField<FUInt64Property>(Property))
        return "integer";

    if (CastField<FFloatProperty>(Property))
        return "number";

    if (CastField<FDoubleProperty>(Property))
        return "number";

    if (CastField<FEnumProperty>(Property))
        return ((FEnumProperty*)Property)->GetEnum()->GetName();

    if (CastField<FBoolProperty>(Property))
        return TEXT("boolean");

    if (CastField<FClassProperty>(Property))
    {
        UClass* Class = ((FClassProperty*)Property)->MetaClass;
        return FString::Printf(TEXT("TSubclassOf<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FSoftObjectProperty>(Property))
    {
        if (((FSoftObjectProperty*)Property)->PropertyClass->IsChildOf(UClass::StaticClass()))
        {
            UClass* Class = ((FSoftClassProperty*)Property)->MetaClass;
            return FString::Printf(TEXT("TSoftClassPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
        }
        UClass* Class = ((FSoftObjectProperty*)Property)->PropertyClass;
        return FString::Printf(TEXT("TSoftObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FObjectProperty>(Property))
    {
        UClass* Class = ((FObjectProperty*)Property)->PropertyClass;
        return FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FWeakObjectProperty>(Property))
    {
        UClass* Class = ((FWeakObjectProperty*)Property)->PropertyClass;
        return FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FLazyObjectProperty>(Property))
    {
        UClass* Class = ((FLazyObjectProperty*)Property)->PropertyClass;
        return FString::Printf(TEXT("TLazyObjectPtr<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FInterfaceProperty>(Property))
    {
        UClass* Class = ((FInterfaceProperty*)Property)->InterfaceClass;
        return FString::Printf(TEXT("TScriptInterface<%s%s>"), Class->GetPrefixCPP(), *Class->GetName());
    }

    if (CastField<FNameProperty>(Property))
        return "string";

    if (CastField<FStrProperty>(Property))
        return "string";

    if (CastField<FTextProperty>(Property))
        return "string";

    if (CastField<FArrayProperty>(Property))
    {
        FProperty* Inner = ((FArrayProperty*)Property)->Inner;
        return FString::Printf(TEXT("TArray<%s>"), *GetTypeName(Inner));
    }

    if (CastField<FMapProperty>(Property))
    {
        FProperty* KeyProp = ((FMapProperty*)Property)->KeyProp;
        FProperty* ValueProp = ((FMapProperty*)Property)->ValueProp;
        return FString::Printf(TEXT("TMap<%s, %s>"), *GetTypeName(KeyProp), *GetTypeName(ValueProp));
    }

    if (CastField<FSetProperty>(Property))
    {
        FProperty* ElementProp = ((FSetProperty*)Property)->ElementProp;
        return FString::Printf(TEXT("TSet<%s>"), *GetTypeName(ElementProp));
    }

    if (CastField<FStructProperty>(Property))
        return ((FStructProperty*)Property)->Struct->GetStructCPPName();

    if (CastField<FDelegateProperty>(Property))
        return "Delegate";

    if (CastField<FMulticastDelegateProperty>(Property))
        return "MulticastDelegate";

    return "Unknown";
}

FString UnLua::IntelliSense::EscapeComments(const FString Comments)
{
    return Comments;
}

bool UnLua::IntelliSense::IsValid(const UFunction* Function)
{
    if (!Function)
        return false;

    if (Function->HasAnyFunctionFlags(FUNC_UbergraphFunction))
        return false;

    if (!UEdGraphSchema_K2::CanUserKismetCallFunction(Function))
        return false;

    const FString Name = Function->GetName();
    if (Name.Len() == 0)
        return false;

    if (Name.Contains(" "))
        return false;

    return true;
}
