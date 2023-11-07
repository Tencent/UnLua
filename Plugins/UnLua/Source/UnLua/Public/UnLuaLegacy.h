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

#include "Runtime/Launch/Resources/Version.h"
#include "UnLuaBase.h"
#include "UnLuaTemplate.h"
#include "LuaValue.h"
#include "LuaEnv.h"

namespace UnLua
{
    /**
     * lstring wrapper
     */
    struct FlStringWrapper
    {
        const char* V;
        uint32 Len;
    };

    /**
     * Push values to the stack.
     */
    template <typename T>
    int32 Push(lua_State* L, T* V, bool bCopy = false);

    template <typename T>
    int32 Push(lua_State* L, T&& V, bool bCopy);

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    int32 Push(lua_State* L, SmartPtrType<T, Mode> V, bool bCopy = false);

    template <typename T>
    TSharedPtr<ITypeInterface> GetTypeInterface();

    FORCEINLINE int32 Push(lua_State* L, int8 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, int16 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, int32 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, int64 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, long V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, uint8 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, uint16 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, uint32 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, uint64 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, unsigned long V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, float V, bool bCopy = false)
    {
        lua_pushnumber(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, double V, bool bCopy = false)
    {
        lua_pushnumber(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, bool V, bool bCopy = false)
    {
        lua_pushboolean(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, char* V, bool bCopy = false)
    {
        lua_pushstring(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const char* V, bool bCopy = false)
    {
        lua_pushstring(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FString& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FString& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FString&& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FName& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FName& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FName&& V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FText& V, bool bCopy = false)
    {
#if UNLUA_ENABLE_FTEXT
        const auto Userdata = NewTypedUserdata(L, FText);
        new(Userdata) FText(V);
#else
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
#endif
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FText& V, bool bCopy = false)
    {
#if UNLUA_ENABLE_FTEXT
        const auto Userdata = NewTypedUserdata(L, FText);
#else
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
#endif
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FText&& V, bool bCopy = false)
    {
#if UNLUA_ENABLE_FTEXT
        const auto Userdata = NewTypedUserdata(L, FText);
#else
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
#endif
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, void* V, bool bCopy = false)
    {
        lua_pushlightuserdata(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const void* V, bool bCopy = false)
    {
        lua_pushlightuserdata(L, (void*)V);
        return 1;
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State* L, TSubclassOf<T>& V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State* L, const TSubclassOf<T>& V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State* L, TSubclassOf<T>&& V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, TArray<T, Allocator>& V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, const TArray<T, Allocator>& V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, TArray<T, Allocator>&& V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, TSet<T, KeyFunc, Allocator>& V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, const TSet<T, KeyFunc, Allocator>& V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State* L, TSet<T, KeyFunc, Allocator>&& V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State* L, TMap<KeyType, ValueType, Allocator, KeyFunc>& V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State* L, const TMap<KeyType, ValueType, Allocator, KeyFunc>& V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State* L, TMap<KeyType, ValueType, Allocator, KeyFunc>&& V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    FORCEINLINE int32 Push(lua_State* L, FlStringWrapper& V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FlStringWrapper& V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FlStringWrapper&& V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaIndex& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FLuaIndex& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaIndex&& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaValue& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FLuaValue& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaValue&& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaTable& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, const FLuaTable& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State* L, FLuaTable&& V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    /**
     * Check value type from stack
     */

    template <typename T> bool IsType(lua_State* L, int32 Index, TType<T>)
    {
        UE_LOG(LogTemp, Log, TEXT("Unsupported Type Check"));
        return false;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<int8>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<int16>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<int32>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<int64>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<long>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<uint8>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<uint16>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<uint32>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<uint64>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<unsigned long>)
    {
        return lua_isinteger(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<float>)
    {
        return lua_isnumber(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<double>)
    {
        return lua_isnumber(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<bool>)
    {
        return lua_isboolean(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<const char*>)
    {
        return lua_isstring(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<FString>)
    {
        return lua_isstring(L, Index) != 0;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<FName>)
    {
        return lua_isstring(L, Index) != 0;
    }

#if !UNLUA_ENABLE_FTEXT
    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<FText>)
    {
        return lua_isstring(L, Index) != 0;
    }
#endif

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<UObject*>)
    {
        return (UnLua::GetUObject(L, Index) != nullptr);
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index)) != nullptr;
    }

    FORCEINLINE bool IsType(lua_State* L, int32 Index, TType<const UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index)) != nullptr;
    }

    /**
     * Get value from the stack.
     */
    template <typename T> T Get(lua_State* L, int32 Index, TType<T>);

    template <typename T> T* Get(lua_State* L, int32 Index, TType<T*>);

    template <typename T> T& Get(lua_State* L, int32 Index, TType<T&>);

    template <typename T> TSubclassOf<T> Get(lua_State* L, int32 Index, TType<TSubclassOf<T>>);
    template <typename T> TSubclassOf<T>* Get(lua_State* L, int32 Index, TType<TSubclassOf<T>*>);
    template <typename T> TSubclassOf<T>& Get(lua_State* L, int32 Index, TType<TSubclassOf<T>&>);
    template <typename T> const TSubclassOf<T>& Get(lua_State* L, int32 Index, TType<const TSubclassOf<T>&>);

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode> Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode>* Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>*>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode>& Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>&>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> const SmartPtrType<T, Mode>& Get(lua_State* L, int32 Index, TType<const SmartPtrType<T, Mode>&>);

    template <typename T, typename Allocator> TArray<T, Allocator> Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>>);
    template <typename T, typename Allocator> TArray<T, Allocator>* Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>*>);
    template <typename T, typename Allocator> TArray<T, Allocator>& Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>&>);
    template <typename T, typename Allocator> const TArray<T, Allocator>& Get(lua_State* L, int32 Index, TType<const TArray<T, Allocator>&>);

    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator> Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>>);
    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator>* Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>*>);
    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator>& Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>&>);
    template <typename T, typename KeyFunc, typename Allocator> const TSet<T, KeyFunc, Allocator>& Get(lua_State* L, int32 Index, TType<const TSet<T, KeyFunc, Allocator>&>);

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc> Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc>* Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>*>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> const TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State* L, int32 Index, TType<const TMap<KeyType, ValueType, Allocator, KeyFunc>&>);

    FORCEINLINE int8 Get(lua_State* L, int32 Index, TType<int8>)
    {
        return (int8)lua_tointeger(L, Index);
    }

    FORCEINLINE int16 Get(lua_State* L, int32 Index, TType<int16>)
    {
        return (int16)lua_tointeger(L, Index);
    }

    FORCEINLINE int32 Get(lua_State* L, int32 Index, TType<int32>)
    {
        return (int32)lua_tointeger(L, Index);
    }

    FORCEINLINE int64 Get(lua_State* L, int32 Index, TType<int64>)
    {
        return (int64)lua_tointeger(L, Index);
    }

    FORCEINLINE long Get(lua_State* L, int32 Index, TType<long>)
    {
        return (long)lua_tointeger(L, Index);
    }

    FORCEINLINE uint8 Get(lua_State* L, int32 Index, TType<uint8>)
    {
        return (uint8)lua_tointeger(L, Index);
    }

    FORCEINLINE uint16 Get(lua_State* L, int32 Index, TType<uint16>)
    {
        return (uint16)lua_tointeger(L, Index);
    }

    FORCEINLINE uint32 Get(lua_State* L, int32 Index, TType<uint32>)
    {
        return (uint32)lua_tointeger(L, Index);
    }

    FORCEINLINE uint64 Get(lua_State* L, int32 Index, TType<uint64>)
    {
        return (uint64)lua_tointeger(L, Index);
    }

    FORCEINLINE unsigned long Get(lua_State* L, int32 Index, TType<unsigned long>)
    {
        return (unsigned long)lua_tointeger(L, Index);
    }

    FORCEINLINE float Get(lua_State* L, int32 Index, TType<float>)
    {
        return (float)lua_tonumber(L, Index);
    }

    FORCEINLINE double Get(lua_State* L, int32 Index, TType<double>)
    {
        return (double)lua_tonumber(L, Index);
    }

    FORCEINLINE bool Get(lua_State* L, int32 Index, TType<bool>)
    {
        return lua_toboolean(L, Index) != 0;
    }

    FORCEINLINE const char* Get(lua_State* L, int32 Index, TType<const char*>)
    {
        return lua_tostring(L, Index);
    }

    FORCEINLINE FString Get(lua_State* L, int32 Index, TType<FString>)
    {
        return UTF8_TO_TCHAR(lua_tostring(L, Index));
    }

    FORCEINLINE FName Get(lua_State* L, int32 Index, TType<FName>)
    {
        return FName(lua_tostring(L, Index));
    }

#if !UNLUA_ENABLE_FTEXT
    FORCEINLINE FText Get(lua_State* L, int32 Index, TType<FText>)
    {
        return FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, Index)));
    }
#endif

    FORCEINLINE UObject* Get(lua_State* L, int32 Index, TType<UObject*>)
    {
        return UnLua::GetUObject(L, Index);
    }

    FORCEINLINE const UObject* Get(lua_State* L, int32 Index, TType<const UObject*>)
    {
        return UnLua::GetUObject(L, Index);
    }

    FORCEINLINE UClass* Get(lua_State* L, int32 Index, TType<UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index));
    }

    FORCEINLINE const UClass* Get(lua_State* L, int32 Index, TType<const UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index));
    }

    /**
     * Push arguments for Lua function
     */
    template <bool bCopy> FORCEINLINE int32 PushArgs(lua_State* L)
    {
        return 0;
    }

    template <bool bCopy, typename T1, typename... T2>
    FORCEINLINE int32 PushArgs(lua_State* L, T1&& V1, T2&&... V2)
    {
        const int32 Ret1 = UnLua::Push(L, Forward<T1>(V1), bCopy);
        const int32 Ret2 = PushArgs<bCopy>(L, Forward<T2>(V2)...);
        return Ret1 + Ret2;
    }

    /**
     * Internal helper to call a Lua function. Used internally only. Please do not use!
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallFunctionInternal(lua_State* L, T&&... Args)
    {
        // make sure the function is on the top of the stack, and the message handler is below the function
        int32 MessageHandlerIdx = lua_gettop(L) - 1;
        check(MessageHandlerIdx > 0);
        int32 NumArgs = PushArgs<false>(L, Forward<T>(Args)...);
        int32 Code = lua_pcall(L, NumArgs, LUA_MULTRET, MessageHandlerIdx);
        int32 TopIdx = lua_gettop(L);
        if (Code == LUA_OK)
        {
            int32 NumResults = TopIdx - MessageHandlerIdx;
            lua_remove(L, MessageHandlerIdx);
            FLuaRetValues Result(L, NumResults);
            return Result;    // MoveTemp(Result);
        }
        lua_pop(L, TopIdx - MessageHandlerIdx + 1);
        return FLuaRetValues(L, INDEX_NONE);
    }

    /**
     * Call Lua global function.
     *
     * @param FuncName - global Lua function name
     * @param Args - function arguments
     * @return - wrapper of all return values
     */
    template <typename... T>
    FLuaRetValues Call(lua_State* L, const char* FuncName, T&&... Args)
    {
        const auto Env = FLuaEnv::FindEnv(L);
        if (!L || !FuncName)
        {
            return FLuaRetValues(Env, INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);

        lua_getglobal(L, FuncName);
        if (lua_isfunction(L, -1) == false)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Global function %s doesn't exist!"), UTF8_TO_TCHAR(FuncName));
            lua_pop(L, 2);
            return FLuaRetValues(L, INDEX_NONE);
        }

        return CallFunctionInternal(L, Forward<T>(Args)...);
    }

    /**
     * Call function in global Lua table.
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallTableFunc(lua_State* L, const char* TableName, const char* FuncName, T&&... Args)
    {
        const auto Env = FLuaEnv::FindEnv(L);
        if (!L || !TableName || !FuncName)
        {
            return FLuaRetValues(Env, INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);

        int32 Type = lua_getglobal(L, TableName);
        if (Type != LUA_TTABLE)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Global table %s doesn't exist!"), UTF8_TO_TCHAR(TableName));
            lua_pop(L, 2);
            return FLuaRetValues(Env, INDEX_NONE);
        }
        Type = lua_getfield(L, -1, FuncName);
        if (Type != LUA_TFUNCTION)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Function %s of global table %s doesn't exist!"), UTF8_TO_TCHAR(FuncName), UTF8_TO_TCHAR(TableName));
            lua_pop(L, 3);
            return FLuaRetValues(Env, INDEX_NONE);
        }
        lua_remove(L, -2);

        return CallFunctionInternal(L, Forward<T>(Args)...);
    }

    /**
     * Call function in Lua table.
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallTableFunc(lua_State* L, FLuaTable& LuaTable, const char* FuncName, T&&... Args)
    {
        const auto Env = FLuaEnv::FindEnv(L);
        if (!Env || !FuncName)
        {
            return FLuaRetValues(Env, INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);

        int32 Type = lua_getfield(L, LuaTable.GetIndex(), FuncName);
        if (Type != LUA_TFUNCTION)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Function %s doesn't exist!"), UTF8_TO_TCHAR(FuncName));
            lua_pop(L, 2);
            return FLuaRetValues(L, INDEX_NONE);
        }

        return CallFunctionInternal(L, Forward<T>(Args)...);
    }

    template <typename T>
    FORCEINLINE T FLuaValue::Value() const
    {
        return UnLua::Get(L, Index, TType<T>());
    }

    template <typename T>
    FORCEINLINE FLuaValue::operator T() const
    {
        return UnLua::Get(L, Index, TType<T>());
    }

    template <typename T, typename Allocator>
    FORCEINLINE FLuaValue::operator TArray<T, Allocator>& () const
    {
        return UnLua::Get(L, Index, TType<TArray<T, Allocator>&>());
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE FLuaValue::operator TSet<T, KeyFunc, Allocator>& () const
    {
        return UnLua::Get(L, Index, TType<TSet<T, KeyFunc, Allocator>&>());
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE FLuaValue::operator TMap<KeyType, ValueType, Allocator, KeyFunc>& () const
    {
        return UnLua::Get(L, Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>());
    }

    /**
     * Call function in Lua table
     */
    template <typename... T>
    FLuaRetValues FLuaTable::Call(const char* FuncName, T&&... Args) const
    {
        if (!FuncName)
        {
            return FLuaRetValues(L, INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);

        lua_pushstring(L, FuncName);
        int32 Type = lua_gettable(L, Index);
        if (Type != LUA_TFUNCTION)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Function %s of table doesn't exist!"), UTF8_TO_TCHAR(FuncName));
            lua_pop(L, 2);
            return FLuaRetValues(L, INDEX_NONE);
        }

        return CallFunctionInternal(L, Forward<T>(Args)...);
    }


    /**
     * Call function
     */
    template <typename... T>
    FLuaRetValues FLuaFunction::Call(T&&... Args) const
    {
        if (!IsValid())
        {
            return FLuaRetValues(L, INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);
        lua_rawgeti(L, LUA_REGISTRYINDEX, FunctionRef);
        return CallFunctionInternal(L, Forward<T>(Args)...);
    }


    /**
     * true/false type
     */
    struct FTrue {};
    struct FFalse {};


    /**
    * Typed interface to manage a C++ type
    */
    template <typename T>
    struct TTypeInterface : public ITypeInterface
    {
        virtual bool IsValid() const override { return true; }

        virtual bool IsPODType() const override { return TIsPODType<T>::Value; }

        virtual bool IsTriviallyDestructible() const override
        {
            static_assert(TIsDestructible<T>::Value, "type must be destructible!");
            return TIsTriviallyDestructible<T>::Value;
        }

        virtual int32 GetSize() const override { return sizeof(T); }
        virtual int32 GetAlignment() const override { return alignof(T); }
        virtual int32 GetOffset() const override { return 0; }

        virtual uint32 GetValueTypeHash(const void* Src) const override
        {
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22)
            static_assert(TModels<CGetTypeHashable, T>::Value, "type must support GetTypeHash()!");
#else
            static_assert(THasGetTypeHash<T>::Value, "type must support GetTypeHash()!");
#endif
            return GetTypeHash(*((const T*)Src));
        }

        virtual void Initialize(void* Dest) const override { FMemory::Memzero(Dest, sizeof(T)); }

        virtual void Destruct(void* Dest) const override
        {
            static_assert(TIsDestructible<T>::Value, "type must be destructible!");
            DestructInternal((T*)Dest, typename TChooseClass<TIsTriviallyDestructible<T>::Value, FTrue, FFalse>::Result());
        }

        virtual void Copy(void* Dest, const void* Src) const override
        {
            static_assert(TIsCopyConstructible<T>::Value, "type must be copy constructible!");
            CopyInternal((T*)Dest, (const T*)Src, typename TChooseClass<TIsTriviallyCopyConstructible<T>::Value, FTrue, FFalse>::Result());
        }

        virtual bool Identical(const void* A, const void* B) const override
        {
            return IdenticalInternal((const T*)A, (const T*)B, typename TChooseClass<THasEqualityOperator<T>::Value, FTrue, FFalse>::Result());
        }

        virtual FString GetName() const override { return FString(TType<typename TDecay<T>::Type>::GetName()); }
        virtual FProperty* GetUProperty() const override { return nullptr; }

        // interfaces of ITypeOps
        virtual void ReadValue_InContainer(lua_State* L, const void* ContainerPtr, bool bCreateCopy) const override
        {
            ReadValue(L, ContainerPtr, bCreateCopy);
        }

        virtual void ReadValue(lua_State* L, const void* ValuePtr, bool bCreateCopy) const override
        {
            UnLua::Push(L, *((T*)ValuePtr), bCreateCopy);
        }

        virtual bool WriteValue_InContainer(lua_State* L, void* ContainerPtr, int32 IndexInStack, bool bCreateCopy) const override
        {
            return WriteValue(L, ContainerPtr, IndexInStack, bCreateCopy);
        }

        virtual bool WriteValue(lua_State* L, void* ValuePtr, int32 IndexInStack, bool bCreateCopy) const override
        {
            static_assert(TIsCopyConstructible<T>::Value, "type must be copy constructible!");
            T V = UnLua::Get(L, IndexInStack, TType<T>());
            CopyInternal((T*)ValuePtr, &V, typename TChooseClass<TIsTriviallyCopyConstructible<T>::Value, FTrue, FFalse>::Result());
            return false;
        }

    private:
        void CopyInternal(T* Dest, const T* Src, FFalse NotTrivial) const { new(Dest) T(*Src); }
        void CopyInternal(T* Dest, const T* Src, FTrue Trivial) const { FMemory::Memcpy(Dest, Src, sizeof(T)); }
        void DestructInternal(T* Dest, FFalse NotTrivial) const { Dest->~T(); }
        void DestructInternal(T* Dest, FTrue Trivial) const {}
        bool IdenticalInternal(const T* A, const T* B, FFalse) const { return false; }
        bool IdenticalInternal(const T* A, const T* B, FTrue) const { return *A == *B; }
    };


    template <typename T>
    struct TTypeInterface<T*> : public ITypeInterface
    {
        virtual bool IsValid() const override { return true; }
        virtual bool IsPODType() const override { return true; }
        virtual bool IsTriviallyDestructible() const override { return true; }
        virtual int32 GetSize() const override { return sizeof(T*); }
        virtual int32 GetAlignment() const override { return alignof(T*); }
        virtual int32 GetOffset() const override { return 0; }
        virtual uint32 GetValueTypeHash(const void* Src) const override { return PointerHash(*((T**)Src)); }
        virtual void Initialize(void* Dest) const override { FMemory::Memzero(Dest, sizeof(T*)); }
        virtual void Destruct(void* Dest) const override {}
        virtual void Copy(void* Dest, const void* Src) const override { *((T**)Dest) = *((T**)Src); }
        virtual bool Identical(const void* A, const void* B) const override { return *((T**)A) == *((T**)B); }
        virtual FString GetName() const override { return FString(TType<typename TDecay<T>::Type>::GetName()); }
        virtual FProperty* GetUProperty() const override { return nullptr; }

        // interfaces of ITypeOps
        virtual void ReadValue_InContainer(lua_State* L, const void* ContainerPtr, bool bCreateCopy) const override
        {
            ReadValue(L, ContainerPtr, bCreateCopy);
        }

        virtual void ReadValue(lua_State* L, const void* ValuePtr, bool bCreateCopy) const override
        {
            UnLua::Push(L, *((T**)ValuePtr), bCreateCopy);
        }

        virtual bool WriteValue_InContainer(lua_State* L, void* ContainerPtr, int32 IndexInStack, bool bCreateCopy = true) const override
        {
            return WriteValue(L, ContainerPtr, IndexInStack, bCreateCopy);
        }

        virtual bool WriteValue(lua_State* L, void* ValuePtr, int32 IndexInStack, bool bCreateCopy) const override
        {
            T* V = UnLua::Get(L, IndexInStack, TType<T*>());
            *((T**)ValuePtr) = V;
            return false;
        }
    };

    template <typename T>
    FORCEINLINE TSharedPtr<ITypeInterface> GetTypeInterface()
    {
        static TSharedPtr<ITypeInterface> TypeInterface(new TTypeInterface<T>);
        return TypeInterface;
    }

    /**
     * Helper for pointer operation
     */
    template <typename T, bool IsUObject = TPointerIsConvertibleFromTo<typename TDecay<T>::Type, UObject>::Value>
    struct TPointerHelper
    {
        static int32 Push(lua_State* L, T* V)
        {
            return UnLua::PushPointer(L, (void*)V, TType<typename TDecay<T>::Type>::GetName());
        }

        static T* Get(lua_State* L, int32 Index)
        {
            T* V = (T*)UnLua::GetPointer(L, Index);
            return V;
        }
    };

    template <typename T>
    struct TPointerHelper<T, true>
    {
        static int32 Push(lua_State* L, T* V)
        {
            return UnLua::PushUObject(L, (UObject*)V);
        }

        static T* Get(lua_State* L, int32 Index)
        {
            T* V = Cast<T>(UnLua::GetUObject(L, Index));
            return V;
        }
    };


    /**
     * Helper for generic type operation
     */
    template <typename T, bool IsEnum = TIsEnum<typename TRemoveReference<T>::Type>::Value>
    struct TGenericTypeHelper
    {
        static int32 Push(lua_State* L, T&& V, bool bCopy)
        {
            if (bCopy)
            {
                void* Userdata = UnLua::NewUserdata(L, sizeof(typename TDecay<T>::Type), TType<typename TDecay<T>::Type>::GetName(), alignof(typename TDecay<T>::Type));
                if (!Userdata)
                {
                    return 0;
                }
                new(Userdata) typename TDecay<T>::Type(V);
                return 1;
            }
            return TPointerHelper<typename TRemoveReference<T>::Type>::Push(L, &V);
        }

        static T Get(lua_State* L, int32 Index)
        {
            T* V = (T*)UnLua::GetPointer(L, Index);
            return *V;
        }
    };

    template <typename T>
    struct TGenericTypeHelper<T, true>
    {
        static int32 Push(lua_State* L, T&& V, bool bCopy)
        {
            lua_pushinteger(L, (lua_Integer)V);
            return 1;
        }

        static T Get(lua_State* L, int32 Index)
        {
            return (T)lua_tointeger(L, Index);
        }
    };


    /**
     * Push values to the stack.
     */
    template <typename T>
    FORCEINLINE int32 Push(lua_State* L, T* V, bool bCopy)
    {
        return TPointerHelper<T>::Push(L, V);
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State* L, T&& V, bool bCopy)
    {
        return TGenericTypeHelper<T>::Push(L, Forward<T>(V), bCopy);
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE_DEBUGGABLE int32 Push(lua_State* L, SmartPtrType<T, Mode> V, bool bCopy)
    {
        if (V.IsValid())
        {
            char MetatableName[256];
            memset(MetatableName, 0, sizeof(MetatableName));
            FCStringAnsi::Snprintf(MetatableName, sizeof(MetatableName), "%s%s", TType<SmartPtrType<T, Mode>>::GetName(), TType<T>::GetName());
            void* Userdata = UnLua::NewSmartPointer(L, sizeof(SmartPtrType<T, Mode>), MetatableName);
            new(Userdata) SmartPtrType<T, Mode>(V);
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }


    /**
     * Get value from the stack.
     */
    template <typename T>
    FORCEINLINE T Get(lua_State* L, int32 Index, TType<T>)
    {
        return TGenericTypeHelper<T>::Get(L, Index);
    }

    template <typename T>
    FORCEINLINE T* Get(lua_State* L, int32 Index, TType<T*>)
    {
        return TPointerHelper<T>::Get(L, Index);
    }

    template <typename T>
    FORCEINLINE T& Get(lua_State* L, int32 Index, TType<T&>)
    {
        T* V = TPointerHelper<T>::Get(L, Index);
        return *V;
    }

    template <typename T>
    FORCEINLINE TSubclassOf<T> Get(lua_State* L, int32 Index, TType<TSubclassOf<T>>)
    {
        TSubclassOf<T>* Class = (TSubclassOf<T>*)lua_touserdata(L, Index);
        return Class ? *Class : TSubclassOf<T>();
    }

    template <typename T>
    FORCEINLINE TSubclassOf<T>* Get(lua_State* L, int32 Index, TType<TSubclassOf<T>*>)
    {
        return (TSubclassOf<T>*)lua_touserdata(L, Index);
    }

    template <typename T>
    FORCEINLINE_DEBUGGABLE TSubclassOf<T>& Get(lua_State* L, int32 Index, TType<TSubclassOf<T>&>)
    {
        TSubclassOf<T>* Class = (TSubclassOf<T>*)lua_touserdata(L, Index);
        if (!Class)
        {
            static TSubclassOf<T> DefaultClass;
            return DefaultClass;
        }
        return *Class;
    }

    template <typename T>
    FORCEINLINE const TSubclassOf<T>& Get(lua_State* L, int32 Index, TType<const TSubclassOf<T>&>)
    {
        return UnLua::Get(L, Index, TType<TSubclassOf<T>&>());
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE SmartPtrType<T, Mode> Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>>)
    {
        SmartPtrType<T, Mode>* SmartPtr = (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
        return SmartPtr ? *SmartPtr : SmartPtrType<T, Mode>();
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE SmartPtrType<T, Mode>* Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>*>)
    {
        return (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE_DEBUGGABLE SmartPtrType<T, Mode>& Get(lua_State* L, int32 Index, TType<SmartPtrType<T, Mode>&>)
    {
        SmartPtrType<T, Mode>* SmartPtr = (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
        if (!SmartPtr)
        {
            static SmartPtrType<T, Mode> DefaultPtr(nullptr);
            return DefaultPtr;
        }
        return *SmartPtr;
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE const SmartPtrType<T, Mode>& Get(lua_State* L, int32 Index, TType<const SmartPtrType<T, Mode>&>)
    {
        return UnLua::Get(L, Index, TType<SmartPtrType<T, Mode>&>());
    }

    template <typename T, typename Allocator>
    FORCEINLINE TArray<T, Allocator> Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>>)
    {
        TArray<T, Allocator>* Array = (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
        return Array ? *Array : TArray<T, Allocator>();
    }

    template <typename T, typename Allocator>
    FORCEINLINE TArray<T, Allocator>* Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>*>)
    {
        return (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
    }

    template <typename T, typename Allocator>
    FORCEINLINE_DEBUGGABLE TArray<T, Allocator>& Get(lua_State* L, int32 Index, TType<TArray<T, Allocator>&>)
    {
        TArray<T, Allocator>* Array = (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
        if (!Array)
        {
            static TArray<T, Allocator> DefaultArray;
            return DefaultArray;
        }
        return *Array;
    }

    template <typename T, typename Allocator>
    FORCEINLINE const TArray<T, Allocator>& Get(lua_State* L, int32 Index, TType<const TArray<T, Allocator>&>)
    {
        return UnLua::Get(L, Index, TType<TArray<T, Allocator>&>());
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE TSet<T, KeyFunc, Allocator> Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>>)
    {
        TSet<T, KeyFunc, Allocator>* Set = (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
        return Set ? *Set : TSet<T, KeyFunc, Allocator>();
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE TSet<T, KeyFunc, Allocator>* Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>*>)
    {
        return (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE_DEBUGGABLE TSet<T, KeyFunc, Allocator>& Get(lua_State* L, int32 Index, TType<TSet<T, KeyFunc, Allocator>&>)
    {
        TSet<T, KeyFunc, Allocator>* Set = (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
        if (!Set)
        {
            static TSet<T, KeyFunc, Allocator> DefaultSet;
            return DefaultSet;
        }
        return *Set;
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE const TSet<T, KeyFunc, Allocator>& Get(lua_State* L, int32 Index, TType<const TSet<T, KeyFunc, Allocator>&>)
    {
        return UnLua::Get(L, Index, TType<TSet<T, KeyFunc, Allocator>&>());
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE TMap<KeyType, ValueType, Allocator, KeyFunc> Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>>)
    {
        TMap<KeyType, ValueType, Allocator, KeyFunc>* Map = (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
        return Map ? *Map : TMap<KeyType, ValueType, Allocator, KeyFunc>();
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE TMap<KeyType, ValueType, Allocator, KeyFunc>* Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>*>)
    {
        return (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE_DEBUGGABLE TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State* L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>)
    {
        TMap<KeyType, ValueType, Allocator, KeyFunc>* Map = (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
        if (!Map)
        {
            static TMap<KeyType, ValueType, Allocator, KeyFunc> DefaultMap;
            return DefaultMap;
        }
        return *Map;
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE const TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State* L, int32 Index, TType<const TMap<KeyType, ValueType, Allocator, KeyFunc>&>)
    {
        return UnLua::Get(L, Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>());
    }
} // namespace UnLua