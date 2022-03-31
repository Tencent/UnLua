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
#include "LuaEnv.h"
#include "ReflectionUtils/PropertyDesc.h"
#include "LuaFunction.generated.h"

UCLASS()
class UNLUA_API ULuaFunction : public UFunction
{
    GENERATED_BODY()

public:
    UnLua::FLuaEnv* Env;

    static bool Override(UFunction* Function, UClass* Outer, UnLua::FLuaEnv* LuaEnv, FName NewName);

    void Initialize();
    
    void Call(UObject* Context, FFrame& Stack, RESULT_DECL);

private:
    static FOutParmRec* FindOutParamRec(FOutParmRec* OutParam, FProperty* OutProperty)
    {
        while (OutParam)
        {
            if (OutParam->Property == OutProperty)
            {
                return OutParam;
            }
            OutParam = OutParam->NextOutParm;
        }
        return nullptr;
    }
    
    bool CallLuaInternal(lua_State *L, void *InParams, FOutParmRec *OutParams, RESULT_DECL) const;

    FORCEINLINE void SkipCodes(FFrame& Stack, void* Params) const;
    
    TWeakObjectPtr<UFunction> Overridden;
    int32 LuaRef;

    // TODO: refactor below
#if ENABLE_PERSISTENT_PARAM_BUFFER
    void *Buffer;
#endif
#if !SUPPORTS_RPC_CALL
    FOutParmRec *OutParmRec;
#endif
    TArray<FPropertyDesc*> Properties;
    TArray<int32> OutPropertyIndices;
    // FParameterCollection *DefaultParams;
    int32 ReturnPropertyIndex;
    int32 LatentPropertyIndex;
    uint8 NumRefProperties;
    uint8 NumCalls;                 // RECURSE_LIMIT is 120 or 250 which is less than 256, so use a byte...
    uint8 bStaticFunc : 1;
    uint8 bInterfaceFunc : 1;
    uint8 bHasDelegateParams : 1;
};
