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

#include "UnLuaBase.h"

namespace UnLua
{
    UNLUA_API void ExportClass(IExportedClass* Class);

    UNLUA_API void ExportEnum(IExportedEnum* Enum);

    UNLUA_API void ExportFunction(IExportedFunction* Function);

    UNLUA_API void AddType(FString Name, TSharedPtr<ITypeInterface> TypeInterface);

    UNLUA_API TMap<FString, IExportedClass*> GetExportedReflectedClasses();

    UNLUA_API TMap<FString, IExportedClass*> GetExportedNonReflectedClasses();

    UNLUA_API TArray<IExportedEnum*> GetExportedEnums();

    UNLUA_API TArray<IExportedFunction*> GetExportedFunctions();

    UNLUA_API IExportedClass* FindExportedClass(FString Name);

    UNLUA_API IExportedClass* FindExportedReflectedClass(FString Name);

    UNLUA_API IExportedClass* FindExportedNonReflectedClass(FString Name);

    UNLUA_API TSharedPtr<ITypeInterface> FindTypeInterface(FString Name);
}
