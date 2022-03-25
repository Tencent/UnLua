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

#include "Binding.h"

namespace UnLua
{
    struct FExported
    {
        TArray<IExportedEnum*> Enums;
        TArray<IExportedFunction*> Functions;
        TMap<FString, IExportedClass*> ReflectedClasses;
        TMap<FString, IExportedClass*> NonReflectedClasses;
        TMap<FString, TSharedPtr<ITypeInterface>> Types;
    };

    FExported* GetExported()
    {
        static FExported Exported;
        return &Exported;
    }

    void ExportClass(IExportedClass* Class)
    {
        if (Class->IsReflected())
            GetExported()->ReflectedClasses.Add(Class->GetName(), Class);
        else
            GetExported()->NonReflectedClasses.Add(Class->GetName(), Class);
    }

    void ExportEnum(IExportedEnum* Enum)
    {
        GetExported()->Enums.Add(Enum);
    }

    void ExportFunction(IExportedFunction* Function)
    {
        GetExported()->Functions.Add(Function);
    }

    void AddType(FString Name, TSharedPtr<ITypeInterface> TypeInterface)
    {
        if (!ensure(!Name.IsEmpty() && TypeInterface))
            return;
        GetExported()->Types.Add(Name, TypeInterface);
    }

    TMap<FString, IExportedClass*> GetExportedReflectedClasses()
    {
        return GetExported()->ReflectedClasses;
    }

    TMap<FString, IExportedClass*> GetExportedNonReflectedClasses()
    {
        return GetExported()->NonReflectedClasses;
    }

    TArray<IExportedEnum*> GetExportedEnums()
    {
        return GetExported()->Enums;
    }

    TArray<IExportedFunction*> GetExportedFunctions()
    {
        return GetExported()->Functions;
    }

    IExportedClass* FindExportedClass(const FString Name)
    {
        auto Class = GetExported()->ReflectedClasses.FindRef(Name);
        if (Class)
        {
            return Class;
        }
        Class = GetExported()->NonReflectedClasses.FindRef(Name);
        return Class;
    }

    IExportedClass* FindExportedReflectedClass(FString Name)
    {
        const auto Class = GetExported()->ReflectedClasses.FindRef(Name);
        return Class;
    }

    IExportedClass* FindExportedNonReflectedClass(FString Name)
    {
        const auto Class = GetExported()->NonReflectedClasses.FindRef(Name);
        return Class;
    }

    TSharedPtr<ITypeInterface> FindTypeInterface(FString Name)
    {
        return GetExported()->Types.FindRef(Name);
    }
}
