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

struct lua_State;

class UNLUA_API FUnLuaDelegates
{
public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaStateCreated, lua_State*);
    DECLARE_MULTICAST_DELEGATE(FOnLuaContextInitialized);
    DECLARE_MULTICAST_DELEGATE(FOnPreStaticallyExport);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaContextCleanup, bool);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectBinded, UObjectBaseUtility*);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectUnbinded, UObjectBaseUtility*);

    DECLARE_DELEGATE_RetVal_OneParam(int32, FGenericLuaDelegate, lua_State*);
    DECLARE_DELEGATE_RetVal_FourParams(bool, FCustomLuaFileLoader, UnLua::FLuaEnv&, const FString&, TArray<uint8>&, FString&);

    static FOnLuaStateCreated OnLuaStateCreated;
    static FOnLuaContextInitialized OnLuaContextInitialized;
    static FOnLuaContextCleanup OnPreLuaContextCleanup;
    static FOnLuaContextCleanup OnPostLuaContextCleanup;
    static FOnPreStaticallyExport OnPreStaticallyExport;

    static FOnObjectBinded OnObjectBinded;
    static FOnObjectUnbinded OnObjectUnbinded;

    static FGenericLuaDelegate HotfixLua;
    static FGenericLuaDelegate ReportLuaCallError;
    static FGenericLuaDelegate ConfigureLuaGC;
    
    static FCustomLuaFileLoader CustomLoadLuaFile;
};
