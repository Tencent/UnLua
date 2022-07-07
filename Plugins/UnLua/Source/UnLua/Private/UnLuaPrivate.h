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

#define UNLUA_LOG(L, CategoryName, Verbosity, Format, ...) \
    {\
    	FString LogMsg = FString::Printf(Format, ##__VA_ARGS__);\
        luaL_traceback(L, L, "", 0); \
        UE_LOG(LogUnLua, Log, TEXT("%s\n%s"),*LogMsg,UTF8_TO_TCHAR(lua_tostring(L,-1))); \
        lua_pop(L,1); \
    }

#define UNLUA_LOGWARNING(L, CategoryName, Verbosity, Format, ...) \
    {\
    	FString LogMsg = FString::Printf(Format, ##__VA_ARGS__);\
        luaL_traceback(L, L, "", 0); \
        UE_LOG(LogUnLua, Warning, TEXT("%s\n%s"),*LogMsg,UTF8_TO_TCHAR(lua_tostring(L,-1))); \
        lua_pop(L,1); \
    }

#define UNLUA_LOGERROR(L, CategoryName, Verbosity, Format, ...) \
    {\
    	FString LogMsg = FString::Printf(Format, ##__VA_ARGS__);\
        luaL_traceback(L, L, "", 0); \
        UE_LOG(LogUnLua, Error, TEXT("%s\n%s"),*LogMsg,UTF8_TO_TCHAR(lua_tostring(L,-1))); \
        lua_pop(L,1); \
    }

#if STATS
DECLARE_STATS_GROUP(TEXT("UnLua"), STATGROUP_UnLua, STATCAT_Advanced);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Lua Memory"), STAT_UnLua_Lua_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Persistent Parameter Buffer Memory"), STAT_UnLua_PersistentParamBuffer_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
DECLARE_MEMORY_STAT_EXTERN(TEXT("OutParmRec Memory"), STAT_UnLua_OutParmRec_Memory, STATGROUP_UnLua, /*UNLUA_API*/);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Container Element Cache Memory"), STAT_UnLua_ContainerElementCache_Memory, STATGROUP_UnLua, /*UNLUA_API*/);

#define UNLUA_STAT_MEMORY_ALLOC(Pointer, CounterName) \
    const auto _AllocedSize = FMemory::GetAllocSize(Pointer); \
    INC_MEMORY_STAT_BY(STAT_UnLua_##CounterName##_Memory, _AllocedSize);

#define UNLUA_STAT_MEMORY_FREE(PointerName, CounterName) \
    const auto _FreedSize = FMemory::GetAllocSize(PointerName); \
    DEC_MEMORY_STAT_BY(STAT_UnLua_##CounterName##_Memory, _FreedSize);

#define UNLUA_STAT_MEMORY_REALLOC(Pointer, NewPointer, CounterName) \
    struct FReallocGuard { \
        uint32 OldSize; \
        void* Ptr; \
        void** NewPtr; \
        ~FReallocGuard() { \
            const auto NewSize = FMemory::GetAllocSize(*NewPtr); \
            if (NewSize > OldSize) \
                INC_MEMORY_STAT_BY(STAT_UnLua_##CounterName##_Memory, NewSize - OldSize) \
            else \
                DEC_MEMORY_STAT_BY(STAT_UnLua_##CounterName##_Memory, OldSize - NewSize) \
        } \
    } _ReallocGuard; \
    _ReallocGuard.OldSize = FMemory::GetAllocSize(Pointer); \
    _ReallocGuard.Ptr = Pointer; \
    _ReallocGuard.NewPtr = &NewPointer; \

#else

#define UNLUA_STAT_MEMORY_ALLOC(Pointer, CounterName)
#define UNLUA_STAT_MEMORY_FREE(PointerName, CounterName)
#define UNLUA_STAT_MEMORY_REALLOC(Pointer, NewPointer, CounterName)

#endif

UNLUA_API extern FString GLuaSrcRelativePath;
UNLUA_API extern FString GLuaSrcFullPath;
