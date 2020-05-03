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

#include "EnumDesc.h"
#include "ClassDesc.h"
#include "FunctionDesc.h"

#define ENABLE_CALL_OVERRIDDEN_FUNCTION 1           // option to call overridden UFunction

/**
 * Reflection registry
 */
class FReflectionRegistry
{
public:
    ~FReflectionRegistry() { Cleanup(); }

    void Cleanup();

    template <typename CharType>
    FClassDesc* FindClass(const CharType *InName)
    {
        FClassDesc **ClassDesc = Name2Classes.Find(FName(InName));
        return ClassDesc ? *ClassDesc : nullptr;
    }

    bool UnRegisterClass(FClassDesc *ClassDesc);
    FClassDesc* RegisterClass(const TCHAR *InName, TArray<FClassDesc*> *OutChain = nullptr);
    FClassDesc* RegisterClass(UStruct *InStruct, TArray<FClassDesc*> *OutChain = nullptr);

    bool NotifyUObjectDeleted(const UObjectBase *InObject);

    template <typename CharType>
    FEnumDesc* FindEnum(const CharType *InName)
    {
        FEnumDesc **EnumDesc = Enums.Find(FName(InName));
        return EnumDesc ? *EnumDesc : nullptr;
    }

    template <typename CharType>
    bool UnRegisterEnumByName(const CharType *InName)
    {
        FName Name(InName);
        FEnumDesc **EnumDescPtr = Enums.Find(Name);
        if (EnumDescPtr)
        {
            FEnumDesc *EnumDesc = *EnumDescPtr;
            if (EnumDesc && EnumDesc->Release())
            {
                Enums.Remove(Name);
                return true;
            }
        }
        return false;
    }

    FEnumDesc* RegisterEnum(const TCHAR *InName);
    FEnumDesc* RegisterEnum(UEnum *InEnum);

    FFunctionDesc* RegisterFunction(UFunction *InFunction, int32 InFunctionRef = INDEX_NONE);
    bool UnRegisterFunction(UFunction *InFunction);
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    bool AddOverriddenFunction(UFunction *NewFunc, UFunction *OverriddenFunc);
    UFunction* RemoveOverriddenFunction(UFunction *NewFunc);
    UFunction* FindOverriddenFunction(UFunction *NewFunc);
#endif

#if UE_BUILD_DEBUG
    void Debug()
    {
        check(true);
    }
#endif

private:
    FClassDesc* RegisterClassInternal(const FString &ClassName, UStruct *Struct, FClassDesc::EType Type);
    void GetClassChain(FClassDesc *ClassDesc, TArray<FClassDesc*> *OutChain);

    TMap<FName, FClassDesc*> Name2Classes;
    TMap<UStruct*, FClassDesc*> Struct2Classes;
    TMap<UStruct*, FClassDesc*> NonNativeStruct2Classes;
    TMap<FName, FEnumDesc*> Enums;
    TMap<UFunction*, FFunctionDesc*> Functions;
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    TMap<UFunction*, UFunction*> OverriddenFunctions;
#endif
};

extern FReflectionRegistry GReflectionRegistry;
