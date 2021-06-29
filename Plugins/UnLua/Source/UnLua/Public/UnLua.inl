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

#include "Runtime/Launch/Resources/Version.h"

namespace UnLua
{

    template <typename T>
    FORCEINLINE T FLuaValue::Value() const
    {
        return UnLua::Get(UnLua::GetState(), Index, TType<T>());
    }

    template <typename T>
    FORCEINLINE FLuaValue::operator T() const
    {
        return UnLua::Get(UnLua::GetState(), Index, TType<T>());
    }

    template <typename T, typename Allocator>
    FORCEINLINE FLuaValue::operator TArray<T, Allocator>&() const
    {
        return UnLua::Get(UnLua::GetState(), Index, TType<TArray<T, Allocator>&>());
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE FLuaValue::operator TSet<T, KeyFunc, Allocator>&() const
    {
        return UnLua::Get(UnLua::GetState(), Index, TType<TSet<T, KeyFunc, Allocator>&>());
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE FLuaValue::operator TMap<KeyType, ValueType, Allocator, KeyFunc>&() const
    {
        return UnLua::Get(UnLua::GetState(), Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>());
    }

    /**
     * Push arguments for Lua function
     */
    template <bool bCopy> FORCEINLINE int32 PushArgs(lua_State *L)
    {
        return 0;
    }
    
    template <bool bCopy, typename T1, typename... T2>
    FORCEINLINE int32 PushArgs(lua_State *L, T1 &&V1, T2&&... V2)
    {
        return UnLua::Push(L, Forward<T1>(V1), bCopy) + PushArgs<bCopy>(L, Forward<T2>(V2)...);
    }

    
    /**
     * Internal helper to call a Lua function. Used internally only. Please do not use!
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallFunctionInternal(lua_State *L, T&&... Args)
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
            FLuaRetValues Result(NumResults);
            return Result;    // MoveTemp(Result);
        }
        lua_pop(L, TopIdx - MessageHandlerIdx + 1);
        return FLuaRetValues(INDEX_NONE);
    }


    /**
     * Call function in Lua table
     */
    template <typename... T>
    FLuaRetValues FLuaTable::Call(const char *FuncName, T&&... Args) const
    {
        if (!FuncName)
        {
            return FLuaRetValues(INDEX_NONE);
        }

        lua_State *L = UnLua::GetState();
        lua_pushcfunction(L, ReportLuaCallError);

        lua_pushstring(L, FuncName);
        int32 Type = lua_gettable(L, Index);
        if (Type != LUA_TFUNCTION)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Function %s of table doesn't exist!"), ANSI_TO_TCHAR(FuncName));
            lua_pop(L, 2);
            return FLuaRetValues(INDEX_NONE);
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
            return FLuaRetValues(INDEX_NONE);
        }

        lua_State *L = UnLua::GetState();
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
        virtual bool IsPODType() const override { return TIsPODType<T>::Value; }

        virtual bool IsTriviallyDestructible() const override
        {
            static_assert(TIsDestructible<T>::Value, "type must be destructible!");
            return TIsTriviallyDestructible<T>::Value;
        }

        virtual int32 GetSize() const override { return sizeof(T); }
        virtual int32 GetAlignment() const override { return alignof(T); }
        virtual int32 GetOffset() const override { return 0; }

        virtual uint32 GetValueTypeHash(const void *Src) const override
        {
#if ENGINE_MINOR_VERSION > 22
            static_assert(TModels<CGetTypeHashable, T>::Value, "type must support GetTypeHash()!");
#else
            static_assert(THasGetTypeHash<T>::Value, "type must support GetTypeHash()!");
#endif
            return GetTypeHash(*((const T*)Src));
        }

        virtual void Initialize(void *Dest) const override { FMemory::Memzero(Dest, sizeof(T)); }

        virtual void Destruct(void *Dest) const override
        {
            static_assert(TIsDestructible<T>::Value, "type must be destructible!");
            DestructInternal((T*)Dest, typename TChooseClass<TIsTriviallyDestructible<T>::Value, FTrue, FFalse>::Result());
        }

        virtual void Copy(void *Dest, const void *Src) const override
        {
            static_assert(TIsCopyConstructible<T>::Value, "type must be copy constructible!");
            CopyInternal((T*)Dest, (const T*)Src, typename TChooseClass<TIsTriviallyCopyConstructible<T>::Value, FTrue, FFalse>::Result());
        }

        virtual bool Identical(const void *A, const void *B) const override
        {
            static_assert(THasEqualityOperator<T>::Value, "type must support operator==()!");
            return IdenticalInternal((const T*)A, (const T*)B, typename TChooseClass<THasEqualityOperator<T>::Value, FTrue, FFalse>::Result());
        }

        virtual FString GetName() const override { return FString(TType<typename TDecay<T>::Type>::GetName()); }
        virtual FProperty* GetUProperty() const override { return nullptr; }

        // interfaces of ITypeOps
        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override
        {
            UnLua::Push(L, *((T*)ContainerPtr), bCreateCopy);
        }

        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override
        {
            static_assert(TIsCopyConstructible<T>::Value, "type must be copy constructible!");
            T V = UnLua::Get(L, IndexInStack, TType<T>());
            CopyInternal((T*)ContainerPtr, &V, typename TChooseClass<TIsTriviallyCopyConstructible<T>::Value, FTrue, FFalse>::Result());
        }

    private:
        void CopyInternal(T *Dest, const T *Src, FFalse NotTrivial) const { new(Dest) T(*Src); }
        void CopyInternal(T *Dest, const T *Src, FTrue Trivial) const { FMemory::Memcpy(Dest, Src, sizeof(T)); }
        void DestructInternal(T *Dest, FFalse NotTrivial) const { Dest->~T(); }
        void DestructInternal(T *Dest, FTrue Trivial) const {}
        bool IdenticalInternal(const T *A, const T *B, FFalse) const { return false; }
        bool IdenticalInternal(const T *A, const T *B, FTrue) const { return *A == *B; }
    };


    template <typename T>
    struct TTypeInterface<T*> : public ITypeInterface
    {
        virtual bool IsPODType() const override { return true; }
        virtual bool IsTriviallyDestructible() const override { return true; }
        virtual int32 GetSize() const override { return sizeof(T*); }
        virtual int32 GetAlignment() const override { return alignof(T*); }
        virtual int32 GetOffset() const override { return 0; }
        virtual uint32 GetValueTypeHash(const void *Src) const override { return PointerHash(*((T**)Src)); }
        virtual void Initialize(void *Dest) const override { FMemory::Memzero(Dest, sizeof(T*)); }
        virtual void Destruct(void *Dest) const override {}
        virtual void Copy(void *Dest, const void *Src) const override { *((T**)Dest) = *((T**)Src); }
        virtual bool Identical(const void *A, const void *B) const override { return *((T**)A) == *((T**)B); }
        virtual FString GetName() const override { return FString(TType<typename TDecay<T>::Type>::GetName()); }
        virtual FProperty* GetUProperty() const override { return nullptr; }

        // interfaces of ITypeOps
        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override
        {
            UnLua::Push(L, *((T**)ContainerPtr), bCreateCopy);
        }

        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override
        {
            T *V = UnLua::Get(L, IndexInStack, TType<T*>());
            *((T**)ContainerPtr) = V;
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
        static int32 Push(lua_State *L, T *V)
        {
            return UnLua::PushPointer(L, (void*)V, TType<typename TDecay<T>::Type>::GetName());
        }

        static T* Get(lua_State *L, int32 Index)
        {
            T *V = (T*)UnLua::GetPointer(L, Index);
            return V;
        }
    };

    template <typename T>
    struct TPointerHelper<T, true>
    {
        static int32 Push(lua_State *L, T *V)
        {
            return UnLua::PushUObject(L, (UObject*)V);
        }

        static T* Get(lua_State *L, int32 Index)
        {
            T *V = Cast<T>(UnLua::GetUObject(L, Index));
            return V;
        }
    };


    /**
     * Helper for generic type operation
     */
    template <typename T, bool IsEnum = TIsEnum<T>::Value>
    struct TGenericTypeHelper
    {
        static int32 Push(lua_State *L, T &&V, bool bCopy)
        {
            if (bCopy)
            {
                void *Userdata = UnLua::NewUserdata(L, sizeof(typename TDecay<T>::Type), TType<typename TDecay<T>::Type>::GetName(), alignof(typename TDecay<T>::Type));
                if (!Userdata)
                {
                    return 0;
                }
                new(Userdata) typename TDecay<T>::Type(V);
                return 1;
            }
            return TPointerHelper<typename TRemoveReference<T>::Type>::Push(L, &V);
        }

        static T Get(lua_State *L, int32 Index)
        {
            T *V = (T*)UnLua::GetPointer(L, Index);
            return *V;
        }
    };

    template <typename T>
    struct TGenericTypeHelper<T, true>
    {
        static int32 Push(lua_State *L, T &&V, bool bCopy)
        {
            lua_pushinteger(L, (lua_Integer)V);
            return 1;
        }

        static T Get(lua_State *L, int32 Index)
        {
            return (T)lua_tointeger(L, Index);
        }
    };


    /**
     * Push values to the stack.
     */
    template <typename T>
    FORCEINLINE int32 Push(lua_State *L, T *V, bool bCopy)
    {
        return TPointerHelper<T>::Push(L, V);
    }

    template <typename T>
    FORCEINLINE int32 Push(lua_State *L, T &&V, bool bCopy)
    {
        return TGenericTypeHelper<T>::Push(L, Forward<T>(V), bCopy);
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE_DEBUGGABLE int32 Push(lua_State *L, SmartPtrType<T, Mode> V, bool bCopy)
    {
        if (V.IsValid())
        {
            char MetatableName[256];
            memset(MetatableName, 0, sizeof(MetatableName));
            FCStringAnsi::Snprintf(MetatableName, sizeof(MetatableName), "%s%s", TType<SmartPtrType<T, Mode>>::GetName(), TType<T>::GetName());
            void *Userdata = UnLua::NewSmartPointer(L, sizeof(SmartPtrType<T, Mode>), MetatableName);
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
    FORCEINLINE T Get(lua_State *L, int32 Index, TType<T>)
    {
        return TGenericTypeHelper<T>::Get(L, Index);
    }

    template <typename T>
    FORCEINLINE T* Get(lua_State *L, int32 Index, TType<T*>)
    {
        return TPointerHelper<T>::Get(L, Index);
    }

    template <typename T>
    FORCEINLINE T& Get(lua_State *L, int32 Index, TType<T&>)
    {
        T *V = TPointerHelper<T>::Get(L, Index);
        return *V;
    }

    template <typename T>
    FORCEINLINE TSubclassOf<T> Get(lua_State *L, int32 Index, TType<TSubclassOf<T>>)
    {
        TSubclassOf<T> *Class = (TSubclassOf<T>*)lua_touserdata(L, Index);
        return Class ? *Class : TSubclassOf<T>();
    }

    template <typename T>
    FORCEINLINE TSubclassOf<T>* Get(lua_State *L, int32 Index, TType<TSubclassOf<T>*>)
    {
        return (TSubclassOf<T>*)lua_touserdata(L, Index);
    }

    template <typename T>
    FORCEINLINE_DEBUGGABLE TSubclassOf<T>& Get(lua_State *L, int32 Index, TType<TSubclassOf<T>&>)
    {
        TSubclassOf<T> *Class = (TSubclassOf<T>*)lua_touserdata(L, Index);
        if (!Class)
        {
            static TSubclassOf<T> DefaultClass;
            return DefaultClass;
        }
        return *Class;
    }

    template <typename T>
    FORCEINLINE const TSubclassOf<T>& Get(lua_State *L, int32 Index, TType<const TSubclassOf<T>&>)
    {
        return UnLua::Get(L, Index, TType<TSubclassOf<T>&>());
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE SmartPtrType<T, Mode> Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>>)
    {
        SmartPtrType<T, Mode> *SmartPtr = (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
        return SmartPtr ? *SmartPtr : SmartPtrType<T, Mode>();
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE SmartPtrType<T, Mode>* Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>*>)
    {
        return (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE_DEBUGGABLE SmartPtrType<T, Mode>& Get(lua_State *L, int32 Index, TType<SmartPtrType<T, Mode>&>)
    {
        SmartPtrType<T, Mode> *SmartPtr = (SmartPtrType<T, Mode>*)UnLua::GetSmartPointer(L, Index);
        if (!SmartPtr)
        {
            static SmartPtrType<T, Mode> DefaultPtr(nullptr);
            return DefaultPtr;
        }
        return *SmartPtr;
    }

    template <template <typename, ESPMode> class SmartPtrType, typename T, ESPMode Mode>
    FORCEINLINE const SmartPtrType<T, Mode>& Get(lua_State *L, int32 Index, TType<const SmartPtrType<T, Mode>&>)
    {
        return UnLua::Get(L, Index, TType<SmartPtrType<T, Mode>&>());
    }

    template <typename T, typename Allocator>
    FORCEINLINE TArray<T, Allocator> Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>>)
    {
        TArray<T, Allocator> *Array = (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
        return Array ? *Array : TArray<T, Allocator>();
    }

    template <typename T, typename Allocator>
    FORCEINLINE TArray<T, Allocator>* Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>*>)
    {
        return (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
    }

    template <typename T, typename Allocator>
    FORCEINLINE_DEBUGGABLE TArray<T, Allocator>& Get(lua_State *L, int32 Index, TType<TArray<T, Allocator>&>)
    {
        TArray<T, Allocator> *Array = (TArray<T, Allocator>*)UnLua::GetArray(L, Index);
        if (!Array)
        {
            static TArray<T, Allocator> DefaultArray;
            return DefaultArray;
        }
        return *Array;
    }

    template <typename T, typename Allocator>
    FORCEINLINE const TArray<T, Allocator>& Get(lua_State *L, int32 Index, TType<const TArray<T, Allocator>&>)
    {
        return UnLua::Get(L, Index, TType<TArray<T, Allocator>&>());
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE TSet<T, KeyFunc, Allocator> Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>>)
    {
        TSet<T, KeyFunc, Allocator> *Set = (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
        return Set ? *Set : TSet<T, KeyFunc, Allocator>();
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE TSet<T, KeyFunc, Allocator>* Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>*>)
    {
        return (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE_DEBUGGABLE TSet<T, KeyFunc, Allocator>& Get(lua_State *L, int32 Index, TType<TSet<T, KeyFunc, Allocator>&>)
    {
        TSet<T, KeyFunc, Allocator> *Set = (TSet<T, KeyFunc, Allocator>*)UnLua::GetSet(L, Index);
        if (!Set)
        {
            static TSet<T, KeyFunc, Allocator> DefaultSet;
            return DefaultSet;
        }
        return *Set;
    }

    template <typename T, typename KeyFunc, typename Allocator>
    FORCEINLINE const TSet<T, KeyFunc, Allocator>& Get(lua_State *L, int32 Index, TType<const TSet<T, KeyFunc, Allocator>&>)
    {
        return UnLua::Get(L, Index, TType<TSet<T, KeyFunc, Allocator>&>());
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE TMap<KeyType, ValueType, Allocator, KeyFunc> Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>>)
    {
        TMap<KeyType, ValueType, Allocator, KeyFunc> *Map = (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
        return Map ? *Map : TMap<KeyType, ValueType, Allocator, KeyFunc>();
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE TMap<KeyType, ValueType, Allocator, KeyFunc>* Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>*>)
    {
        return (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE_DEBUGGABLE TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State *L, int32 Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>)
    {
        TMap<KeyType, ValueType, Allocator, KeyFunc> *Map = (TMap<KeyType, ValueType, Allocator, KeyFunc>*)UnLua::GetMap(L, Index);
        if (!Map)
        {
            static TMap<KeyType, ValueType, Allocator, KeyFunc> DefaultMap;
            return DefaultMap;
        }
        return *Map;
    }

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    FORCEINLINE const TMap<KeyType, ValueType, Allocator, KeyFunc>& Get(lua_State *L, int32 Index, TType<const TMap<KeyType, ValueType, Allocator, KeyFunc>&>)
    {
        return UnLua::Get(L, Index, TType<TMap<KeyType, ValueType, Allocator, KeyFunc>&>());
    }


    /**
     * Call Lua global function.
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues Call(lua_State *L, const char *FuncName, T&&... Args)
    {
        if (!L || !FuncName)
        {
            return FLuaRetValues(INDEX_NONE);
        }
        
        lua_pushcfunction(L, ReportLuaCallError);

        lua_getglobal(L, FuncName);
        if (lua_isfunction(L, -1) == false)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Global function %s doesn't exist!"), ANSI_TO_TCHAR(FuncName));
            lua_pop(L, 2);
            return FLuaRetValues(INDEX_NONE);
        }
        
        return CallFunctionInternal(L, Forward<T>(Args)...);
    }

    /**
     * Call function in global Lua table.
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallTableFunc(lua_State *L, const char *TableName, const char *FuncName,  T&&... Args)
    {
        if (!L || !TableName || !FuncName)
        {
            return FLuaRetValues(INDEX_NONE);
        }

        lua_pushcfunction(L, ReportLuaCallError);

        int32 Type = lua_getglobal(L, TableName);
        if (Type != LUA_TTABLE)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Global table %s doesn't exist!"), ANSI_TO_TCHAR(TableName));
            lua_pop(L, 2);
            return FLuaRetValues(INDEX_NONE);
        }
        Type = lua_getfield(L, -1, FuncName);
        if (Type != LUA_TFUNCTION)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Function %s of global table %s doesn't exist!"), ANSI_TO_TCHAR(FuncName), ANSI_TO_TCHAR(TableName));
            lua_pop(L, 3);
            return FLuaRetValues(INDEX_NONE);
        }
        lua_remove(L, -2);

        return CallFunctionInternal(L, Forward<T>(Args)...);
    }

    /**
     * Call function in Lua Table, Table is sandbox table, self parameter provided.
     *
     * @deprecated use FLuaTable::Call instead!
     */
    template <typename... T>
    FORCEINLINE_DEBUGGABLE FLuaRetValues CallTableFunc(lua_State *L, const FLuaTable& LuaTable, const char *FuncName, T&&... Args)
    {
        return LuaTable.Call(FuncName, LuaTable, Forward<T>(Args)...);
    }

}
