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

#include "CoreMinimal.h"

namespace UnLua
{
    namespace IntelliSense
    {
        UNLUAEDITOR_API FString Get(const UBlueprint* Blueprint);

        UNLUAEDITOR_API FString Get(const UField* Field);

        UNLUAEDITOR_API FString Get(const UEnum* Enum);
        
        UNLUAEDITOR_API FString Get(const UStruct* Struct);

        UNLUAEDITOR_API FString Get(const UScriptStruct* ScriptStruct);
        
        UNLUAEDITOR_API FString Get(const UClass* Class);
        
        UNLUAEDITOR_API FString Get(const UFunction* Function, FString ForceClassName = "");

        UNLUAEDITOR_API FString Get(const FProperty* Property);

        UNLUAEDITOR_API FString GetUE(const TArray<const UField*> AllTypes);
        
        UNLUAEDITOR_API FString GetTypeName(const UObject* Field);

        UNLUAEDITOR_API FString GetTypeName(const FProperty* Property);

        UNLUAEDITOR_API FString EscapeComments(const FString Comments, bool bSingleLine);

        UNLUAEDITOR_API FString EscapeSymbolName(const FString InName);

        UNLUAEDITOR_API bool IsValid(const UFunction* Function);

        UNLUAEDITOR_API bool IsValidFunctionName(const FString Name);
    }
}
