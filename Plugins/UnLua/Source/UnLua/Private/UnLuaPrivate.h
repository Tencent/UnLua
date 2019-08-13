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

#include "CoreUObject.h"
#include "UnLuaBase.h"

#define UNLUA_LOGERROR(L, CategoryName, Verbosity, Format, ...) \
    {\
        UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
        luaL_traceback(L, L, "", 0); \
        lua_error(L); \
    }

#if STATS
DECLARE_STATS_GROUP(TEXT("UnLua"), STATGROUP_UnLua, STATCAT_Advanced);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Lua Memory"), STAT_UnLua_Lua_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Persistent Parameter Buffer Memory"), STAT_UnLua_PersistentParamBuffer_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
DECLARE_MEMORY_STAT_EXTERN(TEXT("OutParmRec Memory"), STAT_UnLua_OutParmRec_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
#endif

UNLUA_API bool HotfixLua();

UNLUA_API extern FString GLuaSrcRelativePath;
UNLUA_API extern FString GLuaSrcFullPath;
