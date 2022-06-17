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

#include "UnLuaDebugBase.h"

namespace UnLua
{

#if WITH_EDITOR
    /**
     * Helper to get readable type name for 'IntelliSense'
     */
    template <typename T>
    struct TTypeIntelliSense
    {
        static FString GetName() { return UTF8_TO_TCHAR(TType<T>::GetName()); }
    };

    template <typename T> struct TTypeIntelliSense<T*> { static FString GetName() { return TTypeIntelliSense<T>::GetName(); } };
    template <> struct TTypeIntelliSense<char*> { static FString GetName() { return TEXT("string"); } };
    template <> struct TTypeIntelliSense<TCHAR*> { static FString GetName() { return TEXT("string"); } };
    template <> struct TTypeIntelliSense<void*> { static FString GetName() { return TEXT("lightuserdata"); } };
    template <> struct TTypeIntelliSense<void> { static FString GetName() { return TEXT(""); } };
    template <> struct TTypeIntelliSense<int8> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<int16> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<int32> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<int64> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<long> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<uint8> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<uint16> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<uint32> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<uint64> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<unsigned long> { static FString GetName() { return TEXT("integer"); } };
    template <> struct TTypeIntelliSense<float> { static FString GetName() { return TEXT("number"); } };
    template <> struct TTypeIntelliSense<double> { static FString GetName() { return TEXT("number"); } };
    template <> struct TTypeIntelliSense<bool> { static FString GetName() { return TEXT("boolean"); } };
    template <> struct TTypeIntelliSense<FString> { static FString GetName() { return TEXT("string"); } };
    template <> struct TTypeIntelliSense<FName> { static FString GetName() { return TEXT("string"); } };
    template <> struct TTypeIntelliSense<FText> { static FString GetName() { return TEXT("string"); } };
    template <typename T> struct TTypeIntelliSense<TSubclassOf<T>> { static FString GetName() { return FString::Printf(TEXT("TSubclassOf<%s>"), *TTypeIntelliSense<T>::GetName()); } };
    template <typename T> struct TTypeIntelliSense<TSoftClassPtr<T>> { static FString GetName() { return FString::Printf(TEXT("TSoftClassPtr<%s>"), *TTypeIntelliSense<T>::GetName()); } };
    template <typename T> struct TTypeIntelliSense<TSoftObjectPtr<T>> { static FString GetName() { return FString::Printf(TEXT("TSoftObjectPtr<%s>"), *TTypeIntelliSense<T>::GetName()); } };
    template <typename T> struct TTypeIntelliSense<TWeakObjectPtr<T>> { static FString GetName() { return FString::Printf(TEXT("TWeakObjectPtr<%s>"), *TTypeIntelliSense<T>::GetName()); } };
    template <typename T> struct TTypeIntelliSense<TLazyObjectPtr<T>> { static FString GetName() { return FString::Printf(TEXT("TLazyObjectPtr<%s>"), *TTypeIntelliSense<T>::GetName()); } };
    template <typename T> struct TTypeIntelliSense<TScriptInterface<T>> { static FString GetName() { return FString::Printf(TEXT("TScriptInterface<%s>"), *TTypeIntelliSense<T>::GetName()); } };

    template <typename T, typename Allocator>
    struct TTypeIntelliSense<TArray<T, Allocator>>
    {
        static FString GetName() { return FString::Printf(TEXT("TArray<%s>"), *TTypeIntelliSense<T>::GetName()); }
    };

    template <typename T, typename KeyFunc, typename Allocator>
    struct TTypeIntelliSense<TSet<T, KeyFunc, Allocator>>
    {
        static FString GetName() { return FString::Printf(TEXT("TSet<%s>"), *TTypeIntelliSense<T>::GetName()); }
    };

    template <typename KeyType, typename ValueType, typename Allocator, typename KeyFunc>
    struct TTypeIntelliSense<TMap<KeyType, ValueType, Allocator, KeyFunc>>
    {
        static FString GetName() { return FString::Printf(TEXT("TMap<%s, %s>"), *TTypeIntelliSense<KeyType>::GetName(), *TTypeIntelliSense<ValueType>::GetName()); }
    };


    /**
     * Helper to get readable argument comment for 'IntelliSense'
     */
    template <typename T, bool IsRef = TIsReferenceType<T>::Value, bool IsConst = TIsConstType<typename TRemoveReference<T>::Type>::Value>
    struct TArgumentComment { static FString Get() { return TEXT(""); } };

    template <typename T>
    struct TArgumentComment<T, true, false> { static FString Get() { return TEXT("@[out]"); } };


    /**
     * Helper to generate IntelliSense for argument
     */
    template <typename... T> struct TArgumentIntelliSense;

    template <typename T1, typename... T2>
    struct TArgumentIntelliSense<T1, T2...>
    {
        static void Generate(FString &Buffer, int32 Index)
        {
            FString TypeName = TTypeIntelliSense<typename TChooseClass<TIsPointer<T1>::Value, typename TDecay<typename TRemovePointer<T1>::Type>::Type*, typename TDecay<T1>::Type>::Result>::GetName();
            Buffer += FString::Printf(TEXT("---@param P%d %s %s\r\n"), Index, *TypeName, *TArgumentComment<T1>::Get());
            TArgumentIntelliSense<T2...>::Generate(Buffer, Index + 1);
        }
    };

    template <> struct TArgumentIntelliSense<> { static void Generate(FString &Buffer, int32 Index) {} };


    /**
     * Generate IntelliSense for arguments
     */
    template <typename RetType, typename... ArgType>
    void GenerateArgsIntelliSense(FString &Buffer, FString &ArgList)
    {
        // parameters
        TArgumentIntelliSense<ArgType...>::Generate(Buffer, 0);

        int32 NumArgs = sizeof...(ArgType);
        if (NumArgs > 0)
        {
            ArgList = TEXT("P0");
            for (int32 i = 1; i < NumArgs; ++i)
            {
                ArgList += FString::Printf(TEXT(", P%d"), i);
            }
        }

        // return 
        FString ReturnTypeName = TTypeIntelliSense<typename TChooseClass<TIsPointer<RetType>::Value, typename TDecay<typename TRemovePointer<RetType>::Type>::Type*, typename TDecay<RetType>::Type>::Result>::GetName();
        if (ReturnTypeName.Len() > 0)
        {
            Buffer += FString::Printf(TEXT("---@return %s\r\n"), *ReturnTypeName);
        }
    }
#endif


    /**
     * Push non-const reference parameters to Lua stack 
     */
    template <typename T, bool IsRef = TIsReferenceType<T>::Value, bool IsConst = TIsConstType<typename TRemoveReference<T>::Type>::Value, bool IsPrimTypeOrPtr = TIsPrimitiveTypeOrPointer<typename TRemoveReference<T>::Type>::Value>
    struct TNonConstRefParamHelper
    {
        static int Push(lua_State *L, T &&V) { return 0; }
    };

    template <typename T>
    struct TNonConstRefParamHelper<T, true, false, true>
    {
        static int Push(lua_State *L, T &&V)
        {
            UnLua::Push(L, Forward<T>(V), false);
            return 1;
        }
    };

    FORCEINLINE int32 PushNonConstRefParam(lua_State *L) { return 0; }

    template <typename T>
    FORCEINLINE int32 PushNonConstRefParam(lua_State *L, T &&V)
    {
        return TNonConstRefParamHelper<T>::Push(L, Forward<T>(V));
    }

    template <typename T1, typename... T2>
    FORCEINLINE int32 PushNonConstRefParam(lua_State *L, T1 &&V1, T2&&... V2)
    {
        const auto Ret1 = PushNonConstRefParam(L, Forward<T1>(V1));
        const auto Ret2 = PushNonConstRefParam(L, Forward<T2>(V2)...); 
        return Ret1 + Ret2;
    }

    template <typename... ArgType, uint32... N>
    FORCEINLINE int32 PushNonConstRefParam(lua_State *L, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>)
    {
        return PushNonConstRefParam(L, Forward<typename TTupleType<N, TTuple<ArgType...>>::Type>(Args.template Get<N>())...);
    }


    /**
     * Invoke function...
     */
    template <typename RetType, typename... ArgType, uint32... N>
    FORCEINLINE_DEBUGGABLE RetType Invoke(const TFunction<RetType(ArgType...)> &Func, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>)
    {
        return Func(Forward<ArgType>(Args.template Get<N>())...);
    }

    template <typename RetType, bool IsClass = TIsClass<RetType>::Value>
    struct TInvokingHelper
    {
        template <typename... ArgType, uint32... N>
        static int32 Invoke(lua_State *L, const TFunction<RetType(ArgType...)> &Func, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...> ParamIndices)
        {
            RetType RetVal = UnLua::Invoke(Func, Args, typename TZeroBasedIndices<sizeof...(ArgType)>::Type());
#if UNLUA_LEGACY_RETURN_ORDER
            int32 Num = PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
#endif
            UnLua::Push(L, Forward<RetType>(RetVal), true);
#if !UNLUA_LEGACY_RETURN_ORDER
            int32 Num = PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
#endif
            return Num + 1;
        }
    };

    template <typename RetType>
    struct TInvokingHelper<RetType, true>
    {
        template <typename... ArgType, uint32... N>
        static int32 Invoke(lua_State *L, const TFunction<RetType(ArgType...)> &Func, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...> ParamIndices)
        {
            int32 Num = 0;
            typename TRemoveConst<RetType>::Type *RetValPtr = lua_gettop(L) > sizeof...(ArgType) ? UnLua::Get(L, sizeof...(ArgType) + 1, TType<typename TRemoveConst<RetType>::Type*>()) : nullptr;
            if (RetValPtr)
            {
                *RetValPtr = UnLua::Invoke(Func, Args, typename TZeroBasedIndices<sizeof...(ArgType)>::Type());
                Num = PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
                lua_pushvalue(L, sizeof...(ArgType) + 1);
            }
            else
            {
                RetType RetVal = UnLua::Invoke(Func, Args, typename TZeroBasedIndices<sizeof...(ArgType)>::Type());
#if UNLUA_LEGACY_RETURN_ORDER
                Num = PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
#endif
                UnLua::Push(L, Forward<typename std::add_lvalue_reference<RetType>::type>(RetVal), true);
#if !UNLUA_LEGACY_RETURN_ORDER
                Num = PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
#endif
            }
            return Num + 1;
        }
    };

    template <> struct TInvokingHelper<void, false>
    {
        template <typename... ArgType, uint32... N>
        static int32 Invoke(lua_State *L, const TFunction<void(ArgType...)> &Func, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...> ParamIndices)
        {
            UnLua::Invoke(Func, Args, typename TZeroBasedIndices<sizeof...(ArgType)>::Type());
            return PushNonConstRefParam<ArgType...>(L, Args, ParamIndices);
        }
    };


    /**
     * Get arguments from Lua stack
     */
    template <typename... T, uint32... N>
    FORCEINLINE_DEBUGGABLE TTuple<T...> GetArgs(lua_State *L, TIndices<N...>, uint32 Offset = 0)
    {
        return TTuple<T...>(UnLua::Get(L, Offset + N, TType<T>())...);
    }


    /**
     * Generic closure to help invoke exported function
     */
    FORCEINLINE int32 InvokeFunction(lua_State *L)
    {
        IExportedFunction *Func = (IExportedFunction*)lua_touserdata(L, lua_upvalueindex(1));
        return Func ? Func->Invoke(L) : 0;
    }


    /**
     * __index, __newindex functions
     */
    FORCEINLINE_DEBUGGABLE int32 GetFieldFromSuperClass(lua_State *L, int32 MetatableIndex, int32 KeyIndex)
    {
#if UE_BUILD_DEBUG
        check(lua_type(L, -1) == LUA_TNIL);
#endif
        lua_pushstring(L, "Super");
        int32 Type = lua_rawget(L, MetatableIndex);
        if (Type == LUA_TTABLE)
        {
            Type = lua_getfield(L, -1, lua_tostring(L, KeyIndex));
            if (Type != LUA_TNIL)
            {
                lua_copy(L, -1, -3);
                lua_copy(L, KeyIndex, -2);
                lua_rawset(L, MetatableIndex);
            }
            else
            {
                lua_pop(L, 2);
            }
        }
        else
        {
#if UE_BUILD_DEBUG
            check(Type == LUA_TNIL);
#endif
            lua_pop(L, 1);
        }
        return Type;
    }

    FORCEINLINE int32 Index(lua_State *L)
    {
        lua_pushvalue(L, 2);        // push key
        int32 Type = lua_rawget(L, lua_upvalueindex(1));
        if (Type == LUA_TNIL)
        {
            Type = GetFieldFromSuperClass(L, lua_upvalueindex(1), 2);
        }
        if (Type == LUA_TUSERDATA)
        {
            TSharedPtr<IExportedProperty> Property = *(TSharedPtr<IExportedProperty>*)lua_touserdata(L, -1);
            void *ContainerPtr = UnLua::GetPointer(L, 1);
            if (ContainerPtr)
            {
                lua_pop(L, 1);
                Property->Read(L, ContainerPtr, false);
            }
        }
        return 1;
    }

    FORCEINLINE int32 NewIndex(lua_State *L)
    {
        lua_pushvalue(L, 2);        // push key
        int32 Type = lua_rawget(L, lua_upvalueindex(1));
        if (Type == LUA_TNIL)
        {
            Type = GetFieldFromSuperClass(L, lua_upvalueindex(1), 2);
        }
        if (Type == LUA_TUSERDATA)
        {
            TSharedPtr<IExportedProperty> Property = *(TSharedPtr<IExportedProperty>*)lua_touserdata(L, -1);
            void *ContainerPtr = UnLua::GetPointer(L, 1);
            if (ContainerPtr)
            {
                Property->Write(L, ContainerPtr, 3);
            }
        }
        lua_pop(L, 1);
        return 0;
    }


    /**
     * Exported constructor
     */
    template <typename ClassType, typename... ArgType>
    TConstructor<ClassType, ArgType...>::TConstructor(const FString &InClassName)
        : ClassName(InClassName)
    {}

    template <typename ClassType, typename... ArgType>
    void TConstructor<ClassType, ArgType...>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack
        lua_pushstring(L, "__call");
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_rawset(L, -3);
    }

    template <typename ClassType, typename... ArgType>
    int32 TConstructor<ClassType, ArgType...>::Invoke(lua_State *L)
    {
        constexpr int Expected = sizeof...(ArgType);
        const int Actual = lua_gettop(L) - 1;
        if (Actual < Expected)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Attempted to call constructor of %s with invalid arguments. %d expected but got %d."), *ClassName, Expected, Actual);
            return 0;
        }

        TTuple<typename TArgTypeTraits<ArgType>::Type...> Args = GetArgs<typename TArgTypeTraits<ArgType>::Type...>(L, typename TOneBasedIndices<Expected>::Type(), 1);
        Construct(L, Args, typename TZeroBasedIndices<Expected>::Type());
        return 1;
    }

    template <typename ClassType, typename... ArgType>
    template <uint32... N> void TConstructor<ClassType, ArgType...>::Construct(lua_State *L, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>)
    {
        void *Userdata = UnLua::NewUserdata(L, sizeof(ClassType), TCHAR_TO_UTF8(*ClassName), alignof(ClassType));
        if (Userdata)
        {
            new(Userdata) ClassType(Args.template Get<N>()...);
        }
    }


    /**
     * Exported smart pointer constructor
     */
    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    TSmartPtrConstructor<SmartPtrType, ClassType, ArgType...>::TSmartPtrConstructor(const FString &InFuncName)
        : FuncName(InFuncName)
    {}

    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    void TSmartPtrConstructor<SmartPtrType, ClassType, ArgType...>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack
        lua_pushstring(L, TCHAR_TO_UTF8(*FuncName));
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_rawset(L, -3);

        char MetatableName[256];
        memset(MetatableName, 0, sizeof(MetatableName));
        FCStringAnsi::Snprintf(MetatableName, sizeof(MetatableName), "%s%s", TType<SmartPtrType>::GetName(), TType<ClassType>::GetName());

        int32 Type = luaL_getmetatable(L, MetatableName);
        lua_pop(L, 1);
        if (Type == LUA_TTABLE)
        {
            return;
        }

        Type = luaL_getmetatable(L, TType<ClassType>::GetName());
        if (Type != LUA_TTABLE)
        {
            lua_pop(L, 1);
            return;
        }

        luaL_newmetatable(L, MetatableName);
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, GarbageCollect);
        lua_rawset(L, -3);
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -3);
        lua_pushcclosure(L, UnLua::Index, 1);
        lua_rawset(L, -3);
        lua_pushstring(L, "__newindex");
        lua_pushvalue(L, -3);
        lua_pushcclosure(L, UnLua::NewIndex, 1);
        lua_rawset(L, -3);
        lua_pop(L, 2);
    }

    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    int32 TSmartPtrConstructor<SmartPtrType, ClassType, ArgType...>::Invoke(lua_State *L)
    {
        constexpr int Expected = sizeof...(ArgType);
        const int Actual = lua_gettop(L); 
        if (Actual < Expected)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Attempted to call constructor of %s with invalid arguments. %d expected but got %d."), *TType<ClassType>::GetName(), Expected, Actual);
            return 0;
        }

        TTuple<typename TArgTypeTraits<ArgType>::Type...> Args = GetArgs<typename TArgTypeTraits<ArgType>::Type...>(L, typename TOneBasedIndices<Expected>::Type());
        Construct(L, Args, typename TZeroBasedIndices<Expected>::Type());
        return 1;
    }

    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    int32 TSmartPtrConstructor<SmartPtrType, ClassType, ArgType...>::GarbageCollect(lua_State *L)
    {
        SmartPtrType *Instance = (SmartPtrType*)lua_touserdata(L, 1);
        if (Instance)
        {
            Instance->~SmartPtrType();
        }
        return 0;
    }

    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    template <uint32... N> void TSmartPtrConstructor<SmartPtrType, ClassType, ArgType...>::Construct(lua_State *L, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>)
    {
        char MetatableName[256];
        memset(MetatableName, 0, sizeof(MetatableName));
        FCStringAnsi::Snprintf(MetatableName, sizeof(MetatableName), "%s%s", TType<SmartPtrType>::GetName(), TType<ClassType>::GetName());
        SmartPtrType SharedPtr = SmartPtrType(new ClassType(Args.template Get<N>()...));
        void *Userdata = UnLua::NewSmartPointer(L, sizeof(SmartPtrType), MetatableName);
        if (Userdata)
        {
            new(Userdata) SmartPtrType(SharedPtr);
        }
    }
    

    /**
    * Exported destructor
    */
    template <typename ClassType>
    void TDestructor<ClassType>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack
        lua_pushstring(L, "__gc");
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_rawset(L, -3);
    }

    template <typename ClassType>
    int32 TDestructor<ClassType>::Invoke(lua_State *L)
    {
        bool bTwoLvlPtr = false;
        ClassType *Instance = (ClassType*)UnLua::GetPointer(L, 1, &bTwoLvlPtr);
        if (Instance && !bTwoLvlPtr)
        {
            Instance->~ClassType();
        }
        return 0;
    }
    

    /**
     * Exported global function
     */
    template <typename RetType, typename... ArgType>
    TExportedFunction<RetType, ArgType...>::TExportedFunction(const FString &InName, RetType(*InFunc)(ArgType...))
        : Name(InName), Func(InFunc)
    {}

    template <typename RetType, typename... ArgType>
    void TExportedFunction<RetType, ArgType...>::Register(lua_State *L)
    {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_setglobal(L, TCHAR_TO_UTF8(*Name));
    }

    template <typename RetType, typename... ArgType>
    int32 TExportedFunction<RetType, ArgType...>::Invoke(lua_State *L)
    {
        constexpr int Expected = sizeof...(ArgType);
        const int Actual = lua_gettop(L); 
        if (Actual < Expected)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Attempted to call %s with invalid arguments. %d expected but got %d."), *Name, Expected, Actual);
            return 0;
        }
        TTuple<typename TArgTypeTraits<ArgType>::Type...> Args = GetArgs<typename TArgTypeTraits<ArgType>::Type...>(L, typename TOneBasedIndices<Expected>::Type());
        return TInvokingHelper<RetType>::Invoke(L, Func, Args, typename TZeroBasedIndices<Expected>::Type());
    }

#if WITH_EDITOR
    template <typename RetType, typename... ArgType>
    void TExportedFunction<RetType, ArgType...>::GenerateIntelliSense(FString &Buffer) const
    {
        // arguments
        FString ArgList;
        GenerateArgsIntelliSense<RetType, ArgType...>(Buffer, ArgList);
        // function definition
        Buffer += FString::Printf(TEXT("function _G.%s(%s) end\r\n\r\n"), *Name, *ArgList);
    }
#endif


    /**
     * Exported member function
     */
    template <typename ClassType, typename RetType, typename... ArgType>
    TExportedMemberFunction<ClassType, RetType, ArgType...>::TExportedMemberFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...), const FString &InClassName)
        : Name(InName), Func([InFunc](ClassType *Obj, ArgType&&... Args) -> RetType { return (Obj->*InFunc)(Forward<ArgType>(Args)...); })
        , ClassName(InClassName)
    {}

    template <typename ClassType, typename RetType, typename... ArgType>
    TExportedMemberFunction<ClassType, RetType, ArgType...>::TExportedMemberFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...) const, const FString &InClassName)
        : Name(InName), Func([InFunc](ClassType *Obj, ArgType&&... Args) -> RetType { return (Obj->*InFunc)(Forward<ArgType>(Args)...); })
        , ClassName(InClassName)
    {}

    template <typename ClassType, typename RetType, typename... ArgType>
    void TExportedMemberFunction<ClassType, RetType, ArgType...>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack
        lua_pushstring(L, TCHAR_TO_UTF8(*Name));
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_rawset(L, -3);
    }

    template <typename ClassType, typename RetType, typename... ArgType>
    int32 TExportedMemberFunction<ClassType, RetType, ArgType...>::Invoke(lua_State *L)
    {
        constexpr int Expected = sizeof...(ArgType) + 1;
        const int Actual = lua_gettop(L); 
        if (Actual < Expected)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Attempted to call %s::%s with invalid arguments. %d expected but got %d."), *ClassName, *Name, Expected, Actual);
            return 0;
        }
        TTuple<ClassType*, typename TArgTypeTraits<ArgType>::Type...> Args = GetArgs<ClassType*, typename TArgTypeTraits<ArgType>::Type...>(L, typename TOneBasedIndices<Expected>::Type());
        if (Args.template Get<0>() == nullptr)
        {
            UE_LOG(LogUnLua, Error, TEXT("Attempted to call %s::%s with nullptr of 'this'."), *ClassName, *Name);
            return 0;
        }
        return TInvokingHelper<RetType>::Invoke(L, Func, Args, typename TOneBasedIndices<sizeof...(ArgType)>::Type());
    }

#if WITH_EDITOR
    template <typename ClassType, typename RetType, typename... ArgType>
    void TExportedMemberFunction<ClassType, RetType, ArgType...>::GenerateIntelliSense(FString &Buffer) const
    {
        Buffer += FString::Printf(TEXT("\r\n\r\n"));

        // arguments
        FString ArgList;
        GenerateArgsIntelliSense<RetType, ArgType...>(Buffer, ArgList);
        // function definition
        Buffer += FString::Printf(TEXT("function %s:%s(%s) end\r\n"), *ClassName, *Name, *ArgList);
    }
#endif


    /**
     * Exported static member function
     */
    template <typename RetType, typename... ArgType>
    TExportedStaticMemberFunction<RetType, ArgType...>::TExportedStaticMemberFunction(const FString &InName, RetType(*InFunc)(ArgType...), const FString &InClassName)
        : Super(InName, InFunc)
        , ClassName(InClassName)
    {}

    template <typename RetType, typename... ArgType>
    void TExportedStaticMemberFunction<RetType, ArgType...>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack
        lua_pushstring(L, TCHAR_TO_UTF8(*Super::Name));
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, InvokeFunction, 1);
        lua_rawset(L, -3);
    }

#if WITH_EDITOR
    template <typename RetType, typename... ArgType>
    void TExportedStaticMemberFunction<RetType, ArgType...>::GenerateIntelliSense(FString &Buffer) const
    {
        Buffer += FString::Printf(TEXT("\r\n\r\n"));

        // arguments
        FString ArgList;
        GenerateArgsIntelliSense<RetType, ArgType...>(Buffer, ArgList);
        // function definition
        Buffer += FString::Printf(TEXT("function %s.%s(%s) end\r\n"), *ClassName, *Super::Name, *ArgList);
    }
#endif


    /**
     * Exported property
     */
    template <typename T>
    TExportedProperty<T>::TExportedProperty(const FString &InName, uint32 InOffset)
        : FExportedProperty(InName, InOffset)
    {}

    template <typename T>
    void TExportedProperty<T>::Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const
    {
        T &V = *((T*)((uint8*)ContainerPtr + Offset));
        UnLua::Push(L, V, bCreateCopy || (TIsClass<T>::Value && (Offset == 0)));
    }

    template <typename T>
    void TExportedProperty<T>::Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const
    {
        *((T*)((uint8*)ContainerPtr + Offset)) = UnLua::Get(L, IndexInStack, TType<typename TArgTypeTraits<T>::Type>());
    }

#if WITH_EDITOR
    template <typename T>
    void TExportedProperty<T>::GenerateIntelliSense(FString &Buffer) const
    {
        FString TypeName = TTypeIntelliSense<typename TDecay<T>::Type>::GetName();
        Buffer += FString::Printf(TEXT("---@field %s %s %s \r\n"), TEXT("public"), *Name, *TypeName);
    }
#endif

    /**
     * Exported static property
     */
    template <typename T>
    TExportedStaticProperty<T>::TExportedStaticProperty(const FString &InName, T* Value)
        : FExportedProperty(InName, 0), Value(Value)
    {}

#if WITH_EDITOR
    template <typename T>
    void TExportedStaticProperty<T>::GenerateIntelliSense(FString &Buffer) const
    {
        FString TypeName = TTypeIntelliSense<typename TDecay<T>::Type>::GetName();
        Buffer += FString::Printf(TEXT("---@field %s %s %s \r\n"), TEXT("public"), *Name, *TypeName);
    }
#endif

    template <typename T>
    void TExportedStaticProperty<T>::Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const
    {
        
    }

    template <typename T>
    void TExportedStaticProperty<T>::Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const
    {
        
    }
    
    template <typename T>
    TExportedArrayProperty<T>::TExportedArrayProperty(const FString &InName, uint32 InOffset, int32 InArrayDim)
        : FExportedProperty(InName, InOffset), ArrayDim(InArrayDim)
    {}

    template <typename T>
    void TExportedArrayProperty<T>::Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const
    {
        lua_newtable(L);
        T *V = (T*)((uint8*)ContainerPtr + Offset);
        // the first element
        lua_pushinteger(L, 1);
        UnLua::Push(L, V[0], bCreateCopy || (TIsClass<T>::Value && (Offset == 0)));
        lua_rawset(L, -3);
        // other elements
        for (int32 i = 1; i < ArrayDim; ++i)
        {
            lua_pushinteger(L, i + 1);
            UnLua::Push(L, V[i], bCreateCopy);
            lua_rawset(L, -3);
        }
    }

    template <typename T>
    void TExportedArrayProperty<T>::Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const
    {
        if (IndexInStack < 0 && IndexInStack > LUA_REGISTRYINDEX)
        {
            int32 Top = lua_gettop(L);
            IndexInStack = Top + IndexInStack + 1;
        }
        T *V = (T*)((uint8*)ContainerPtr + Offset);
        for (int32 i = 0; i < ArrayDim; ++i)
        {
            lua_rawgeti(L, IndexInStack, i + 1);
            V[i] = UnLua::Get(L, -1, TType<typename TArgTypeTraits<T>::Type>());
        }
        lua_pop(L, ArrayDim);
    }

#if WITH_EDITOR
    template <typename T>
    void TExportedArrayProperty<T>::GenerateIntelliSense(FString &Buffer) const
    {
        FString TypeName = TTypeIntelliSense<typename TDecay<T>::Type>::GetName();
        Buffer += FString::Printf(TEXT("---@field %s %s %s[] \r\n"), TEXT("public"), *Name, *TypeName);
    }
#endif

    template <typename ClassType, typename PropertyType>
    struct TPropertyOffset
    {
        TPropertyOffset(PropertyType ClassType::*InProperty)
            : Property(InProperty)
        {}

        union
        {
            PropertyType ClassType::*Property;
            uint32 Offset;
        };
    };

    template <typename ClassType, typename PropertyType, int32 N>
    struct TArrayPropertyOffset
    {
        TArrayPropertyOffset(PropertyType(ClassType::*InProperty)[N])
            : Property(InProperty)
        {}

        union
        {
            PropertyType(ClassType::*Property)[N];
            uint32 Offset;
        };
    };


    /**
     * Exported class base
     */
    template <bool bIsReflected>
    TExportedClassBase<bIsReflected>::TExportedClassBase(const char *InName, const char *InSuperClassName)
        : Name(InName), SuperClassName(InSuperClassName)
    {}

    template <bool bIsReflected>
    void TExportedClassBase<bIsReflected>::Register(lua_State *L)
    {
        // make sure the meta table is on the top of the stack if 'bIsReflected' is true

        const auto ClassName = TCHAR_TO_UTF8(*Name);
        if (!bIsReflected)
        {
            int32 Type = luaL_getmetatable(L, ClassName);
            lua_pop(L, 1);
            if (Type == LUA_TTABLE)
            {
                return;
            }
            else
            {
                if (!SuperClassName.IsEmpty())
                {
                    IExportedClass *SuperClass = UnLua::FindExportedClass(SuperClassName);
                    if (SuperClass)
                    {
                        SuperClass->Register(L);
                    }
                    else
                    {
                        SuperClassName = "";
                    }
                }

                luaL_newmetatable(L, ClassName);

                if (!SuperClassName.IsEmpty())
                {
                    lua_pushstring(L, "Super");
                    Type = luaL_getmetatable(L, TCHAR_TO_UTF8(*SuperClassName));
                    check(Type == LUA_TTABLE);
                    lua_rawset(L, -3);
                }

                lua_pushstring(L, "__index");
                lua_pushvalue(L, -2);
                if (Properties.Num() > 0 || !SuperClassName.IsEmpty())
                {
                    lua_pushcclosure(L, UnLua::Index, 1);
                }
                lua_rawset(L, -3);

                lua_pushstring(L, "__newindex");
                lua_pushvalue(L, -2);
                if (Properties.Num() > 0 || !SuperClassName.IsEmpty())
                {
                    lua_pushcclosure(L, UnLua::NewIndex, 1);
                }
                lua_rawset(L, -3);

                lua_pushvalue(L, -1);                    // set metatable to self
                lua_setmetatable(L, -2);
            }
        }

        for (const auto& Property : Properties)
            Property->Register(L);

        for (const auto& MemberFunc : Functions)
            MemberFunc->Register(L);

        for (const auto& Func : GlueFunctions)
            Func->Register(L);

        if (!bIsReflected)
        {
            lua_getglobal(L, "UE");
            lua_pushstring(L, ClassName);
            lua_pushvalue(L, -3);
            lua_rawset(L, -3);
            lua_pop(L, 2);
        }
    }

    template <bool bIsReflected>
    void TExportedClassBase<bIsReflected>::AddLib(const luaL_Reg *InLib)
    {
        if (InLib)
        {
            while (InLib->name && InLib->func)
            {
                GlueFunctions.Add(new FGlueFunction(UTF8_TO_TCHAR(InLib->name), InLib->func));
                ++InLib;
            }
        }
    }

#if WITH_EDITOR
    template <bool bIsReflected>
    void TExportedClassBase<bIsReflected>::GenerateIntelliSense(FString &Buffer) const
    {
        GenerateIntelliSenseInternal(Buffer, typename TChooseClass<bIsReflected, FTrue, FFalse>::Result());
    }

    template <bool bIsReflected>
    void TExportedClassBase<bIsReflected>::GenerateIntelliSenseInternal(FString &Buffer, FFalse NotReflected) const
    {
        // class name
        Buffer = FString::Printf(TEXT("---@class %s"), *Name);
        if (!SuperClassName.IsEmpty())
            Buffer += TEXT(": ") + SuperClassName;
        Buffer += TEXT("\r\n");

        // fields
        for (const auto& Property : Properties)
        {
            Property->GenerateIntelliSense(Buffer);
        }

        // definition
        Buffer += TEXT("local M = {}\r\n");

        // functions
        for (IExportedFunction *Function : Functions)
        {
            Function->GenerateIntelliSense(Buffer);
        }

        // return
        Buffer += TEXT("\r\n\r\nreturn M\r\n");
    }

    template <bool bIsReflected>
    void TExportedClassBase<bIsReflected>::GenerateIntelliSenseInternal(FString &Buffer, FTrue Reflected) const
    {
        // class name
        Buffer = FString::Printf(TEXT("---@type %s\r\n"), *Name);
        Buffer += TEXT("local M = {}\r\n");

        // fields
        for (const auto& Property : Properties)
        {
            FString Field;
            TArray<FString> OutArray;
            Property->GenerateIntelliSense(Field);
            int32 Num = Field.ParseIntoArray(OutArray, TEXT(" "));
            if (Num >= 4)
            {
                Buffer += FString::Printf(TEXT("\r\n\r\n---@type %s \r\n"), *OutArray[3]);
                Buffer += FString::Printf(TEXT("%s.%s = nil"), *Name, *OutArray[2]);
            }
        }

        // functions
        for (IExportedFunction *Function : Functions)
        {
            Function->GenerateIntelliSense(Buffer);
        }

        // return
        Buffer += TEXT("\r\n\r\nreturn M\r\n");
    }
#endif


    /**
     * Exported class
     */
    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    TExportedClass<bIsReflected, ClassType, CtorArgType...>::TExportedClass(const char *InName, const char *InSuperClassName)
        : FExportedClassBase(InName, InSuperClassName)
    {
        AddDefaultFunctions(typename TChooseClass<bIsReflected, FTrue, FFalse>::Result());
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    bool TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddBitFieldBoolProperty(const FString &InName, uint8 *Buffer)
    {
        for (uint32 Offset = 0; Offset < sizeof(ClassType); ++Offset)
        {
            if (uint8 Mask = Buffer[Offset])
            {
                FExportedClassBase::Properties.Add(MakeShared<FExportedBitFieldBoolProperty>(InName, Offset, Mask));
                return true;
            }
        }
        return false;
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename T> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddProperty(const FString &InName, T ClassType::*Property)
    {
        TPropertyOffset<ClassType, T> PropertyOffset(Property);
        FExportedClassBase::Properties.Add(MakeShared<TExportedProperty<T>>(InName, PropertyOffset.Offset));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename T, int32 N> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddProperty(const FString &InName, T (ClassType::*Property)[N])
    {
        TArrayPropertyOffset<ClassType, T, N> PropertyOffset(Property);
        FExportedClassBase::Properties.Add(MakeShared<TExportedArrayProperty<T>>(InName, PropertyOffset.Offset, N));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename T> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddStaticProperty(const FString &InName, T *Property)
    {
        FExportedClassBase::Properties.Add(MakeShared<TExportedStaticProperty<T>>(InName, Property));
    }
    
    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename RetType, typename... ArgType> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...))
    {
        FExportedClassBase::Functions.Add(new TExportedMemberFunction<ClassType, RetType, ArgType...>(InName, InFunc, FExportedClassBase::Name));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename RetType, typename... ArgType> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...) const)
    {
        FExportedClassBase::Functions.Add(new TExportedMemberFunction<ClassType, RetType, ArgType...>(InName, InFunc, FExportedClassBase::Name));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <typename RetType, typename... ArgType> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddStaticFunction(const FString &InName, RetType(*InFunc)(ArgType...))
    {
        FExportedClassBase::Functions.Add(new TExportedStaticMemberFunction<RetType, ArgType...>(InName, InFunc, FExportedClassBase::Name));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <ESPMode Mode, typename... ArgType> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddSharedPtrConstructor()
    {
        static_assert(TPointerIsConvertibleFromTo<ClassType, UObject>::Value == false, "UObject doesn't support shared ptr");
        FExportedClassBase::Functions.Add(new TSharedPtrConstructor<Mode, ClassType, ArgType...>);
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    template <ESPMode Mode, typename... ArgType> void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddSharedRefConstructor()
    {
        static_assert(TPointerIsConvertibleFromTo<ClassType, UObject>::Value == false, "UObject doesn't support shared ref");
        FExportedClassBase::Functions.Add(new TSharedRefConstructor<Mode, ClassType, ArgType...>);
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddStaticCFunction(const FString &InName, lua_CFunction InFunc)
    {
        FExportedClassBase::GlueFunctions.Add(new FGlueFunction(InName, InFunc));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddDefaultFunctions(FFalse NotReflected)
    {
        AddConstructor(typename TChooseClass<TIsConstructible<ClassType, CtorArgType...>::Value, FTrue, FFalse>::Result());
        AddDestructor(typename TChooseClass<TAnd<TIsDestructible<ClassType>, TNot<TIsTriviallyDestructible<ClassType>>>::Value, FFalse, FTrue>::Result());
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddDefaultFunctions(FTrue Reflected)
    {
        AddDefaultFunctions_Reflected(typename TChooseClass<TPointerIsConvertibleFromTo<ClassType, UObject>::Value, FTrue, FFalse>::Result());
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddDefaultFunctions_Reflected(FFalse NotUObject)
    {
        int32 NumArgs = sizeof...(CtorArgType);
        if (NumArgs > 0)
        {
            AddConstructor(typename TChooseClass<TIsConstructible<ClassType, CtorArgType...>::Value, FTrue, FFalse>::Result());
        }
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddConstructor(FTrue Constructible)
    {
        FExportedClassBase::Functions.Add(new TConstructor<ClassType, CtorArgType...>(FExportedClassBase::Name));
    }

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    void TExportedClass<bIsReflected, ClassType, CtorArgType...>::AddDestructor(FFalse NotTrivial)
    {
        FExportedClassBase::Functions.Add(new TDestructor<ClassType>);
    }

}
