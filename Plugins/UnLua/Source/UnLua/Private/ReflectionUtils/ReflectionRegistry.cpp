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

#include "ReflectionRegistry.h"
#include "PropertyDesc.h"
#include "UnLua.h"

FReflectionRegistry::FReflectionRegistry()
{
    PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddLambda([this]()
    {
        this->PostGarbageCollect();
    });
}

FReflectionRegistry::~FReflectionRegistry()
{
    Cleanup();
    FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);
}

/**
 * Clean up
 */
void FReflectionRegistry::Cleanup()
{
    Functions.Empty();
}

bool FReflectionRegistry::UnRegisterFunction(UFunction* InFunction)
{
    if (InFunction->HasAnyFlags(RF_BeginDestroyed))
        return false;
    TSharedPtr<FFunctionDesc> FunctionDesc;
    return Functions.RemoveAndCopyValue(InFunction, FunctionDesc);
}

bool FReflectionRegistry::IsDescValid(void* Desc, EDescType type)
{   
    return true;
}

bool FReflectionRegistry::IsDescValidWithObjectCheck(void* Desc, EDescType type)
{
    bool bValid = IsDescValid(Desc, type);
    if (bValid)
    {
        switch (type)
        {
        case DESC_CLASS:
            bValid = bValid && ((FClassDesc*)Desc)->IsValid();
            break;
        case DESC_FUNCTION:
            bValid = bValid && ((FFunctionDesc*)Desc)->IsValid();
            break;
        case DESC_PROPERTY:
            bValid = bValid && ((FPropertyDesc*)Desc)->IsValid();
            break;
        case DESC_ENUM:
            bValid = bValid && ((FEnumDesc*)Desc)->IsValid();
            break;
        default:
            bValid = false;
        }
    }

    return bValid;
}

void FReflectionRegistry::PostGarbageCollect()
{
    for (auto It = Functions.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
UNLUA_API FReflectionRegistry GReflectionRegistry;        // global reflection registry
