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
#include "UnLuaTemplate.h"
#include "lua.hpp"

namespace UnLua
{

    /**
     * lstring wrapper
     */
    struct FlStringWrapper
    {
        const char *V;
        uint32 Len;
    };

    /**
     * Lua stack index wrapper
     */
    struct UNLUA_API FLuaIndex
    {
        explicit FLuaIndex(int32 _Index);

        FORCEINLINE int32 GetIndex() const { return Index; }

    protected:
        int32 Index;
    };

    /**
     * Generic Lua value
     */
    struct UNLUA_API FLuaValue : public FLuaIndex
    {
        explicit FLuaValue(int32 _Index);
        explicit FLuaValue(int32 _Index, int32 _Type);

        FORCEINLINE int32 GetType() const { return Type; }

        template <typename T>
        T Value() const;

        template <typename T>
        operator T() const;

        template <typename T, typename Allocator>
        operator TArray<T, Allocator>&() const;

        template <typename T, typename KeyFunc, typename Allocator>
        operator TSet<T, KeyFunc, Allocator>&() const;

        template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
        operator TMap<KeyType, ValueType, Allocator, KeyFunc>&() const;

    private:
        int32 Type;
    };

    /**
     * Lua function return values
     */
    struct UNLUA_API FLuaRetValues
    {
        explicit FLuaRetValues(int32 NumResults);
        FLuaRetValues(FLuaRetValues &&Src);
        ~FLuaRetValues();

        void Pop();
        FORCEINLINE bool IsValid() const { return bValid; }
        FORCEINLINE int32 Num() const { return Values.Num(); }

        const FLuaValue& operator[](int32 i) const;

    private:
        FLuaRetValues(const FLuaRetValues &Src);

        TArray<FLuaValue> Values;
        bool bValid;
    };

    /**
     * Lua table wrapper
     */
    struct UNLUA_API FLuaTable : public FLuaIndex
    {
        explicit FLuaTable(int32 _Index);
        explicit FLuaTable(FLuaValue Value);
        ~FLuaTable();

        void Reset();
        int32 Length() const;

        FLuaValue operator[](int32 i) const;
        FLuaValue operator[](int64 i) const;
        FLuaValue operator[](double d) const;
        FLuaValue operator[](const char *s) const;
        FLuaValue operator[](const void *p) const;
        FLuaValue operator[](FLuaIndex StackIndex) const;
        FLuaValue operator[](FLuaValue Key) const;

        template <typename... T>
        FLuaRetValues Call(const char *FuncName, T&&... Args) const;

        /**
         * Iterate for Array
         */
        bool Iterate(TFunction<void (const lua_Integer& Index, const FLuaValue& Value)> Func) const;

        /**
         * Iterate for Table
         */
        bool Iterate(TFunction<void (const FName& Key, const FLuaValue& Value)> Func) const;

    private:
        mutable int32 PushedValues;
    };

    /**
     * Lua function wrapper
     */
    struct UNLUA_API FLuaFunction
    {
        explicit FLuaFunction(const char *GlobalFuncName);
        explicit FLuaFunction(const char *GlobalTableName, const char *FuncName);
        explicit FLuaFunction(FLuaTable Table, const char *FuncName);
        explicit FLuaFunction(FLuaValue Value);
        ~FLuaFunction();

        FORCEINLINE bool IsValid() const { return FunctionRef != LUA_REFNIL; }

        template <typename... T>
        FLuaRetValues Call(T&&... Args) const;

    private:
        int32 FunctionRef;
    };

    /**
     * Helper to check Lua stack
     */
    struct UNLUA_API FCheckStack
    {
#if !UE_BUILD_SHIPPING
        FCheckStack();
        explicit FCheckStack(int32 _ExpectedTopIncrease);
        ~FCheckStack();

    private:
        int32 OldTop;
        int32 ExpectedTopIncrease;
#endif
    };

    /**
     * Helper to recover Lua stack automatically
     */
    struct UNLUA_API FAutoStack
    {
        FAutoStack();
        ~FAutoStack();

    private:
        int32 OldTop;
    };


    /**
     * Get instance of type interface
     *
     * @return - type interface
     */
    template <typename T> TSharedPtr<ITypeInterface> GetTypeInterface();


    /**
     * Push values to the stack.
     */
    template <typename T> int32 Push(lua_State *L, T *V, bool bCopy = false);

    template <typename T> int32 Push(lua_State *L, T &&V, bool bCopy);

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> int32 Push(lua_State *L, SmartPtrType<T, Mode> V, bool bCopy = false);

    FORCEINLINE int32 Push(lua_State *L, int8 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, int16 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, int32 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, int64 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, long V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, uint8 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, uint16 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, uint32 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, uint64 V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, unsigned long V, bool bCopy = false)
    {
        lua_pushinteger(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, float V, bool bCopy = false)
    {
        lua_pushnumber(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, double V, bool bCopy = false)
    {
        lua_pushnumber(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, bool V, bool bCopy = false)
    {
        lua_pushboolean(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, char *V, bool bCopy = false)
    {
        lua_pushstring(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const char *V, bool bCopy = false)
    {
        lua_pushstring(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FString &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FString &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FString &&V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FName &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FName &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FName &&V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FText &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FText &V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FText &&V, bool bCopy = false)
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*V.ToString()));
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, void *V, bool bCopy = false)
    {
        lua_pushlightuserdata(L, V);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const void *V, bool bCopy = false)
    {
        lua_pushlightuserdata(L, (void*)V);
        return 1;
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State *L, TSubclassOf<T> &V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State *L, const TSubclassOf<T> &V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State *L, TSubclassOf<T> &&V, bool bCopy)
    {
        return UnLua::PushUObject(L, V.Get());
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, TArray<T, Allocator> &V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, const TArray<T, Allocator> &V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, TArray<T, Allocator> &&V, bool bCopy)
    {
        return UnLua::PushArray(L, (const FScriptArray*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, TSet<T, KeyFunc, Allocator> &V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, const TSet<T, KeyFunc, Allocator> &V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE int32 Push(lua_State *L, TSet<T, KeyFunc, Allocator> &&V, bool bCopy)
    {
        return UnLua::PushSet(L, (const FScriptSet*)(&V), GetTypeInterface<T>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State *L, TMap<KeyType, ValueType, Allocator, KeyFunc> &V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State *L, const TMap<KeyType, ValueType, Allocator, KeyFunc> &V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE int32 Push(lua_State *L, TMap<KeyType, ValueType, Allocator, KeyFunc> &&V, bool bCopy)
    {
        return UnLua::PushMap(L, (const FScriptMap*)(&V), GetTypeInterface<KeyType>(), GetTypeInterface<ValueType>(), bCopy);
    }

    FORCEINLINE int32 Push(lua_State *L, FlStringWrapper &V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FlStringWrapper &V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FlStringWrapper &&V, bool bCopy = false)
    {
        lua_pushlstring(L, V.V, V.Len);
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaIndex &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FLuaIndex &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaIndex &&V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaValue &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FLuaValue &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaValue &&V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaTable &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, const FLuaTable &V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    FORCEINLINE int32 Push(lua_State *L, FLuaTable &&V, bool bCopy = false)
    {
        lua_pushvalue(L, V.GetIndex());
        return 1;
    }

    /**
     * Get value from the stack.
     */
    template <typename T> T Get(lua_State *L, int32 Index, TType<T>);

    template <typename T> T* Get(lua_State *L, int32 Index, TType<T*>);

    template <typename T> T& Get(lua_State *L, int32 Index, TType<T&>);

    template <typename T> TSubclassOf<T> Get(lua_State *L, int32 Index, TType<TSubclassOf<T>>);
    template <typename T> TSubclassOf<T>* Get(lua_State *L, int32 Index, TType<TSubclassOf<T>*>);
    template <typename T> TSubclassOf<T>& Get(lua_State *L, int32 Index, TType<TSubclassOf<T>&>);
    template <typename T> const TSubclassOf<T>& Get(lua_State *L, int32 Index, TType<const TSubclassOf<T>&>);

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode> Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode>* Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>*>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> SmartPtrType<T, Mode>& Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>&>);
    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode> const SmartPtrType<T, Mode>& Get(lua_State *L, int32 Index, TType<const SmartPtrType<T, Mode>&>);

    template <typename T, typename Allocator> TArray<T, Allocator> Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>>);
    template <typename T, typename Allocator> TArray<T, Allocator>* Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>*>);
    template <typename T, typename Allocator> TArray<T, Allocator>& Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>&>);
    template <typename T, typename Allocator> const TArray<T, Allocator>& Get(lua_State *L, int32 Index, TType<const TArray<T, Allocator>&>);

    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator> Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>>);
    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator>* Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>*>);
    template <typename T, typename KeyFunc, typename Allocator> TSet<T, KeyFunc, Allocator>& Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>&>);
    template <typename T, typename KeyFunc, typename Allocator> const TSet<T, KeyFunc, Allocator>& Get(lua_State *L, int32 Index, TType<const TSet<T, KeyFunc, Allocator>&>);

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc> Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc>* Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>*>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>);
    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc> const TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State *L, int32 Index, TType<const TMap<KeyType, ValueType, Allocator, KeyFunc>&>);

    FORCEINLINE int8 Get(lua_State *L, int32 Index, TType<int8>)
    {
        return (int8)lua_tointeger(L, Index);
    }

    FORCEINLINE int16 Get(lua_State *L, int32 Index, TType<int16>)
    {
        return (int16)lua_tointeger(L, Index);
    }

    FORCEINLINE int32 Get(lua_State *L, int32 Index, TType<int32>)
    {
        return (int32)lua_tointeger(L, Index);
    }

    FORCEINLINE int64 Get(lua_State *L, int32 Index, TType<int64>)
    {
        return (int64)lua_tointeger(L, Index);
    }

    FORCEINLINE long Get(lua_State *L, int32 Index, TType<long>)
    {
        return (long)lua_tointeger(L, Index);
    }

    FORCEINLINE uint8 Get(lua_State *L, int32 Index, TType<uint8>)
    {
        return (uint8)lua_tointeger(L, Index);
    }

    FORCEINLINE uint16 Get(lua_State *L, int32 Index, TType<uint16>)
    {
        return (uint16)lua_tointeger(L, Index);
    }

    FORCEINLINE uint32 Get(lua_State *L, int32 Index, TType<uint32>)
    {
        return (uint32)lua_tointeger(L, Index);
    }

    FORCEINLINE uint64 Get(lua_State *L, int32 Index, TType<uint64>)
    {
        return (uint64)lua_tointeger(L, Index);
    }

    FORCEINLINE unsigned long Get(lua_State *L, int32 Index, TType<unsigned long>)
    {
        return (unsigned long)lua_tointeger(L, Index);
    }

    FORCEINLINE float Get(lua_State *L, int32 Index, TType<float>)
    {
        return (float)lua_tonumber(L, Index);
    }

    FORCEINLINE double Get(lua_State *L, int32 Index, TType<double>)
    {
        return (double)lua_tonumber(L, Index);
    }

    FORCEINLINE bool Get(lua_State *L, int32 Index, TType<bool>)
    {
        return lua_toboolean(L, Index) != 0;
    }

    FORCEINLINE const char* Get(lua_State *L, int32 Index, TType<const char*>)
    {
        return lua_tostring(L, Index);
    }

    FORCEINLINE FString Get(lua_State *L, int32 Index, TType<FString>)
    {
        return UTF8_TO_TCHAR(lua_tostring(L, Index));
    }

    FORCEINLINE FName Get(lua_State *L, int32 Index, TType<FName>)
    {
        return FName(lua_tostring(L, Index));
    }

    FORCEINLINE FText Get(lua_State *L, int32 Index, TType<FText>)
    {
        return FText::FromString(UTF8_TO_TCHAR(lua_tostring(L, Index)));
    }

    FORCEINLINE UObject* Get(lua_State *L, int32 Index, TType<UObject*>)
    {
        return UnLua::GetUObject(L, Index);
    }

    FORCEINLINE const UObject* Get(lua_State *L, int32 Index, TType<const UObject*>)
    {
        return UnLua::GetUObject(L, Index);
    }

    FORCEINLINE UClass* Get(lua_State *L, int32 Index, TType<UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index));
    }

    FORCEINLINE const UClass* Get(lua_State *L, int32 Index, TType<const UClass*>)
    {
        return Cast<UClass>(UnLua::GetUObject(L, Index));
    }
    
    
    /**
     * Call Lua global function.
     *
     * @param FuncName - global Lua function name
     * @param Args - function arguments
     * @return - wrapper of all return values
     */
    template <typename... T>
    FLuaRetValues Call(lua_State *L, const char *FuncName, T&&... Args);

    /**
     * Call function in global Lua table.
     *
     * @param TableName - global Lua table name
     * @param FuncName - function name
     * @param Args - function arguments
     * @return - wrapper of all return values
     */
    template <typename... T>
    FLuaRetValues CallTableFunc(lua_State *L, const char *TableName, const char *FuncName, T&&... Args);

} // namespace UnLua

/**
 * Macros to add type interfaces
 */
#define ADD_TYPE_INTERFACE(Type) \
    ADD_NAMED_TYPE_INTERFACE(Type, Type)

#define ADD_NAMED_TYPE_INTERFACE(Name, Type) \
    static struct FTypeInterface##Name \
    { \
        FTypeInterface##Name() \
        { \
            UnLua::AddTypeInterface(#Name, UnLua::GetTypeInterface<Type>()); \
        } \
    } TypeInterface##Name;

#include "UnLua.inl"
