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

#include "UnLua.h"
#include "Binding.h"

namespace UnLua
{

    /**
     * Exported constructor
     */
    template <typename ClassType, typename... ArgType>
    struct TConstructor : public IExportedFunction
    {
        explicit TConstructor(const FString &InClassName);

        virtual void Register(lua_State *L) override;
        virtual int32 Invoke(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return TEXT(""); }
        virtual void GenerateIntelliSense(FString &Buffer) const override {}
#endif

    private:
        template <uint32... N>
        void Construct(lua_State *L, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>);

        FString ClassName;
    };

    /**
     * Exported smart pointer constructor
     */
    template <typename SmartPtrType, typename ClassType, typename... ArgType>
    struct TSmartPtrConstructor : public IExportedFunction
    {
        explicit TSmartPtrConstructor(const FString &InFuncName);

        virtual void Register(lua_State *L) override;
        virtual int32 Invoke(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return TEXT(""); }
        virtual void GenerateIntelliSense(FString &Buffer) const override {}
#endif

        static int32 GarbageCollect(lua_State *L);

    protected:
        template <uint32... N>
        void Construct(lua_State *L, TTuple<typename TArgTypeTraits<ArgType>::Type...> &Args, TIndices<N...>);

        FString FuncName;
    };

    template <ESPMode Mode, typename ClassType, typename... ArgType>
    struct TSharedPtrConstructor : public TSmartPtrConstructor<TSharedPtr<ClassType, Mode>, ClassType, ArgType...>
    {
        TSharedPtrConstructor()
            : TSmartPtrConstructor<TSharedPtr<ClassType, Mode>, ClassType, ArgType...>(Mode == ESPMode::NotThreadSafe ? TEXT("SharedPtr") : TEXT("ThreadsafeSharedPtr"))
        {}
    };

    template <ESPMode Mode, typename ClassType, typename... ArgType>
    struct TSharedRefConstructor : public TSmartPtrConstructor<TSharedRef<ClassType, Mode>, ClassType, ArgType...>
    {
        TSharedRefConstructor()
            : TSmartPtrConstructor<TSharedRef<ClassType, Mode>, ClassType, ArgType...>(Mode == ESPMode::NotThreadSafe ? TEXT("SharedRef") : TEXT("ThreadsafeSharedRef"))
        {}
    };

    /**
     * Exported destructor
     */
    template <typename ClassType>
    struct TDestructor : public IExportedFunction
    {
        virtual void Register(lua_State *L) override;
        virtual int32 Invoke(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return TEXT(""); }
        virtual void GenerateIntelliSense(FString &Buffer) const override {}
#endif
    };

    /**
     * Exported glue function
     */
    struct FGlueFunction : public IExportedFunction
    {
        FGlueFunction(const FString &InName, lua_CFunction InFunc)
            : Name(InName), Func(InFunc)
        {}

        virtual void Register(lua_State *L) override
        {
            // make sure the meta table is on the top of the stack
            lua_pushstring(L, TCHAR_TO_UTF8(*Name));
            lua_pushcfunction(L, Func);
            lua_rawset(L, -3);
        }

        virtual int32 Invoke(lua_State *L) override { return 0; }

#if WITH_EDITOR
        virtual FString GetName() const override { return Name; }
        virtual void GenerateIntelliSense(FString &Buffer) const override {}
#endif

    private:
        FString Name;
        lua_CFunction Func;
    };

    /**
     * Exported global function
     */
    template <typename RetType, typename... ArgType>
    struct TExportedFunction : public IExportedFunction
    {
        TExportedFunction(const FString &InName, RetType(*InFunc)(ArgType...));

        virtual void Register(lua_State *L) override;
        virtual int32 Invoke(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return Name; }
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    protected:
        FString Name;
        TFunction<RetType(ArgType...)> Func;
    };

    /**
     * Exported member function
     */
    template <typename ClassType, typename RetType, typename... ArgType>
    struct TExportedMemberFunction : public IExportedFunction
    {
        TExportedMemberFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...), const FString &InClassName);
        TExportedMemberFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...) const, const FString &InClassName);

        virtual void Register(lua_State *L) override;
        virtual int32 Invoke(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return Name; }
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    private:
        FString Name;
        TFunction<RetType(ClassType*, ArgType...)> Func;
        FString ClassName;
    };

    /**
     * Exported static member function
     */
    template <typename RetType, typename... ArgType>
    struct TExportedStaticMemberFunction : public TExportedFunction<RetType, ArgType...>
    {
        typedef TExportedFunction<RetType, ArgType...> Super;

        TExportedStaticMemberFunction(const FString &InName, RetType(*InFunc)(ArgType...), const FString &InClassName);

        virtual void Register(lua_State *L) override;

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    private:
        FString ClassName;
    };


    /**
     * Exported property
     */
    struct FExportedProperty : public IExportedProperty, public TSharedFromThis<FExportedProperty>
    {
        virtual void Register(lua_State *L) override
        {
            // make sure the meta table is on the top of the stack
            lua_pushstring(L, TCHAR_TO_UTF8(*Name));
            const auto Userdata = NewSmartPointer(L, sizeof(TSharedPtr<FExportedProperty>), "TSharedPtr");
            new(Userdata) TSharedPtr<FExportedProperty>(this->AsShared());
            lua_rawset(L, -3);
        }

#if WITH_EDITOR
        virtual FString GetName() const override { return Name; }
#endif

    protected:
        FExportedProperty(const FString &InName, uint32 InOffset)
            : Name(InName), Offset(InOffset)
        {}

        FString Name;
        uint32 Offset;
    };

    struct FExportedBitFieldBoolProperty : public FExportedProperty
    {
        FExportedBitFieldBoolProperty(const FString &InName, uint32 InOffset, uint8 InMask)
            : FExportedProperty(InName, InOffset), Mask(InMask)
        {}

        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override
        {
            bool V = !!(*((uint8*)ContainerPtr + Offset) & Mask);
            UnLua::Push(L, V);
        }

        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override
        {
            bool V = UnLua::Get(L, IndexInStack, TType<bool>());
            uint8 *ValuePtr = (uint8*)ContainerPtr + Offset;
            *ValuePtr = ((*ValuePtr) & ~Mask) | (V ? Mask : 0);
        }

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override
        {
            Buffer += FString::Printf(TEXT("---@field public %s boolean \r\n"), *Name);
        }
#endif

    private:
        uint8 Mask;
    };

    template <typename T>
    struct TExportedProperty : public FExportedProperty
    {
        TExportedProperty(const FString &InName, uint32 InOffset);

        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override;
        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override;

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif
    };

    template <typename T>
    struct TExportedStaticProperty : public FExportedProperty
    {
    public:
        TExportedStaticProperty(const FString &InName, T* Value);

        virtual void Register(lua_State *L) override
        {
            // make sure the meta table is on the top of the stack
            lua_pushstring(L, TCHAR_TO_UTF8(*Name));
            UnLua::Push<T>(L, Value);
            lua_rawset(L, -3);
        }

        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override;
        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override;
        
#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    private:
        T* Value;
    };
    
    template <typename T>
    struct TExportedArrayProperty : public FExportedProperty
    {
        TExportedArrayProperty(const FString &InName, uint32 InOffset, int32 InArrayDim);

        virtual void Read(lua_State *L, const void *ContainerPtr, bool bCreateCopy) const override;
        virtual void Write(lua_State *L, void *ContainerPtr, int32 IndexInStack) const override;

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    protected:
        int32 ArrayDim;
    };


    /**
     * Exported class
     */
    template <bool bIsReflected>
    struct TExportedClassBase : public IExportedClass
    {
        TExportedClassBase(const char *InName, const char *InSuperClassName = nullptr);

        virtual void Register(lua_State *L) override;
        virtual void AddLib(const luaL_Reg *InLib) override;
        virtual bool IsReflected() const override { return bIsReflected; }
        virtual FString GetName() const override { return Name; }

#if WITH_EDITOR
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

    private:
#if WITH_EDITOR
        void GenerateIntelliSenseInternal(FString &Buffer, FFalse NotReflected) const;
        void GenerateIntelliSenseInternal(FString &Buffer, FTrue Reflected) const;
#endif

    protected:
        FString Name;
        FString SuperClassName;
        TArray<TSharedPtr<IExportedProperty>> Properties;
        TArray<IExportedFunction*> Functions;
        TArray<IExportedFunction*> GlueFunctions;
    };

    template <bool bIsReflected, typename ClassType, typename... CtorArgType>
    struct TExportedClass : public TExportedClassBase<bIsReflected>
    {
        typedef TExportedClassBase<bIsReflected> FExportedClassBase;

        TExportedClass(const char *InName, const char *InSuperClassName = nullptr);

        bool AddBitFieldBoolProperty(const FString &InName, uint8 *Buffer);

        template <typename T> void AddProperty(const FString &InName, T ClassType::*Property);
        template <typename T, int32 N> void AddProperty(const FString &InName, T (ClassType::*Property)[N]);
        template <typename T> void AddStaticProperty(const FString &InName, T *Property);

        template <typename RetType, typename... ArgType> void AddFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...));
        template <typename RetType, typename... ArgType> void AddFunction(const FString &InName, RetType(ClassType::*InFunc)(ArgType...) const);
        template <typename RetType, typename... ArgType> void AddStaticFunction(const FString &InName, RetType(*InFunc)(ArgType...));

        template <ESPMode Mode, typename... ArgType> void AddSharedPtrConstructor();
        template <ESPMode Mode, typename... ArgType> void AddSharedRefConstructor();

        void AddStaticCFunction(const FString &InName, lua_CFunction InFunc);

    private:
        void AddDefaultFunctions(FFalse NotReflected);
        void AddDefaultFunctions(FTrue Reflected);

        void AddDefaultFunctions_Reflected(FFalse NotUObject);
        void AddDefaultFunctions_Reflected(FTrue IsUObject) {}

        void AddConstructor(FFalse) {}
        void AddConstructor(FTrue Constructible);

        void AddDestructor(FFalse NotTrivial);
        void AddDestructor(FTrue) {}
    };


    /**
     * Exported enum
     */
    struct UNLUA_API FExportedEnum : public IExportedEnum
    {
        explicit FExportedEnum(const FString &InName)
            : Name(InName)
        {}

        virtual void Register(lua_State *L) override;

#if WITH_EDITOR
        virtual FString GetName() const override { return Name; }
        virtual void GenerateIntelliSense(FString &Buffer) const override;
#endif

        void Add(const TCHAR *Key, int32 Value) { NameValues.Add(Key, Value); }

    protected:
        FString Name;
        TMap<FString, int32> NameValues;
    };

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
UnLua::AddType(#Name, UnLua::GetTypeInterface<Type>()); \
} \
} TypeInterface##Name;

/**
 * Export a class
 */
#define EXPORT_UNTYPED_CLASS(Name, bIsReflected, Lib) \
    struct FExported##Name##Helper \
    { \
        static FExported##Name##Helper StaticInstance; \
        FExported##Name##Helper() \
            : ExportedClass(nullptr) \
        { \
            UnLua::IExportedClass *Class = UnLua::FindExportedClass(#Name); \
            if (!Class) \
            { \
                ExportedClass = new UnLua::TExportedClassBase<bIsReflected>(#Name); \
                UnLua::ExportClass(ExportedClass); \
                Class = ExportedClass; \
            } \
            Class->AddLib(Lib); \
        } \
        ~FExported##Name##Helper() \
        { \
            delete ExportedClass; \
        } \
        UnLua::IExportedClass *ExportedClass; \
    };

#define BEGIN_EXPORT_CLASS(Type, ...) \
    BEGIN_EXPORT_NAMED_CLASS(Type, Type, ##__VA_ARGS__)

#define BEGIN_EXPORT_NAMED_CLASS(Name, Type, ...) \
    DEFINE_NAMED_TYPE(#Name, Type) \
    BEGIN_EXPORT_CLASS_EX(false, Name, , Type, nullptr, ##__VA_ARGS__)

#define BEGIN_EXPORT_REFLECTED_CLASS(Type, ...) \
    BEGIN_EXPORT_CLASS_EX(true, Type, , Type, nullptr, ##__VA_ARGS__)

#define BEGIN_EXPORT_CLASS_WITH_SUPER(Type, SuperType, ...) \
    BEGIN_EXPORT_NAMED_CLASS_WITH_SUPER_NAME(Type, Type, #SuperType, ##__VA_ARGS__)

#define BEGIN_EXPORT_NAMED_CLASS_WITH_SUPER_NAME(TypeName, Type, SuperTypeName, ...) \
    DEFINE_NAMED_TYPE(#TypeName, Type) \
    BEGIN_EXPORT_CLASS_EX(false, TypeName, , Type, SuperTypeName, ##__VA_ARGS__)

#define BEGIN_EXPORT_CLASS_EX(bIsReflected, Name, Suffix, Type, SuperTypeName, ...) \
    struct FExported##Name##Suffix##Helper \
    { \
        typedef Type ClassType; \
        static FExported##Name##Suffix##Helper StaticInstance; \
        UnLua::TExportedClass<bIsReflected, Type, ##__VA_ARGS__> *ExportedClass; \
        ~FExported##Name##Suffix##Helper() \
        { \
            delete ExportedClass; \
        } \
        FExported##Name##Suffix##Helper() \
            : ExportedClass(nullptr) \
        { \
            UnLua::TExportedClass<bIsReflected, Type, ##__VA_ARGS__> *Class = (UnLua::TExportedClass<bIsReflected, Type, ##__VA_ARGS__>*)UnLua::FindExportedClass(#Name); \
            if (!Class) \
            { \
                ExportedClass = new UnLua::TExportedClass<bIsReflected, Type, ##__VA_ARGS__>(#Name, SuperTypeName); \
                UnLua::ExportClass((UnLua::IExportedClass*)ExportedClass); \
                Class = ExportedClass; \
            }

#define ADD_PROPERTY(Property) \
            Class->AddProperty(#Property, &ClassType::Property);

#define ADD_STATIC_PROPERTY(Property) \
            Class->AddStaticProperty(#Property, &ClassType::Property);

#define ADD_BITFIELD_BOOL_PROPERTY(Property) \
            { \
                uint8 Buffer[sizeof(ClassType)] = {0}; \
                ((ClassType*)Buffer)->Property = 1; \
                bool bSuccess = Class->AddBitFieldBoolProperty(#Property, Buffer); \
                check(bSuccess); \
            }

#define ADD_FUNCTION(Function) \
            Class->AddFunction(#Function, &ClassType::Function);

#define ADD_NAMED_FUNCTION(Name, Function) \
            Class->AddFunction(Name, &ClassType::Function);

#define ADD_FUNCTION_EX(Name, RetType, Function, ...) \
            Class->AddFunction<RetType, ##__VA_ARGS__>(Name, (RetType(ClassType::*)(__VA_ARGS__))(&ClassType::Function));

#define ADD_CONST_FUNCTION_EX(Name, RetType, Function, ...) \
            Class->AddFunction<RetType, ##__VA_ARGS__>(Name, (RetType(ClassType::*)(__VA_ARGS__) const)(&ClassType::Function));

#define ADD_STATIC_FUNCTION(Function) \
            Class->AddStaticFunction(#Function, &ClassType::Function);

#define ADD_STATIC_FUNCTION_EX(Name, RetType, Function, ...) \
            Class->AddStaticFunction<RetType, ##__VA_ARGS__>(Name, &ClassType::Function);

#define ADD_EXTERNAL_FUNCTION(RetType, Function, ...) \
            Class->AddStaticFunction<RetType, ##__VA_ARGS__>(#Function, Function);

#define ADD_EXTERNAL_FUNCTION_EX(Name, RetType, Function, ...) \
            Class->AddStaticFunction<RetType, ##__VA_ARGS__>(Name, Function);

#define ADD_STATIC_CFUNTION(Function) \
            Class->AddStaticCFunction(#Function, &ClassType::Function);

#define ADD_NAMED_STATIC_CFUNTION(Name, Function) \
            Class->AddStaticCFunction(Name, &ClassType::Function);

#define ADD_LIB(Lib) \
            Class->AddLib(Lib);

#define ADD_SHARED_PTR_CONSTRUCTOR(Mode, ...) \
            Class->AddSharedPtrConstructor<Mode, ##__VA_ARGS__>();

#define ADD_SHARED_REF_CONSTRUCTOR(Mode, ...) \
            Class->AddSharedRefConstructor<Mode, ##__VA_ARGS__>();

#define END_EXPORT_CLASS(...) \
        } \
    };

#define IMPLEMENT_EXPORTED_CLASS(Name) \
    FExported##Name##Helper FExported##Name##Helper::StaticInstance;

#define IMPLEMENT_EXPORTED_CLASS_EX(Name, Suffix) \
    FExported##Name##Suffix##Helper FExported##Name##Suffix##Helper::StaticInstance;

/**
 * Export a global function
 */
#define EXPORT_FUNCTION(RetType, Function, ...) \
    static struct FExportedFunc##Function : public UnLua::TExportedFunction<RetType, ##__VA_ARGS__> \
    { \
        FExportedFunc##Function(const FString &InName, RetType(*InFunc)(__VA_ARGS__)) \
            : UnLua::TExportedFunction<RetType, ##__VA_ARGS__>(InName, InFunc) \
        { \
            UnLua::ExportFunction(this); \
        } \
    } Exported##Function(#Function, Function);

#define EXPORT_FUNCTION_EX(Name, RetType, Function, ...) \
    static struct FExportedFunc##Name : public UnLua::TExportedFunction<RetType, ##__VA_ARGS__> \
    { \
        FExportedFunc##Name(const FString &InName, RetType(*InFunc)(__VA_ARGS__)) \
            : UnLua::TExportedFunction<RetType, ##__VA_ARGS__>(InName, InFunc) \
        { \
            UnLua::ExportFunction(this); \
        } \
    } Exported##Name(#Name, Function);

/**
 * Export an enum
 */
#define BEGIN_EXPORT_ENUM(Enum) \
    DEFINE_NAMED_TYPE(#Enum, Enum) \
    static struct FExported##Enum : public UnLua::FExportedEnum \
    { \
        typedef Enum EnumType; \
        FExported##Enum(const FString &InName) \
            : UnLua::FExportedEnum(InName) \
        { \
            UnLua::ExportEnum(this);

#define BEGIN_EXPORT_ENUM_EX(Enum, Name) \
    DEFINE_NAMED_TYPE(#Name, Enum) \
    static struct FExported##Name : public UnLua::FExportedEnum \
    { \
        typedef Enum EnumType; \
        FExported##Name(const FString &InName) \
            : UnLua::FExportedEnum(InName) \
        { \
            UnLua::ExportEnum(this);

#define BEGIN_EXPORT_ENUM_EX(Enum, Name) \
    DEFINE_NAMED_TYPE(#Name, Enum) \
    static struct FExported##Name : public UnLua::FExportedEnum \
    { \
        typedef Enum EnumType; \
        FExported##Name(const FString &InName) \
            : UnLua::FExportedEnum(InName) \
        { \
            UnLua::ExportEnum(this);

#define ADD_ENUM_VALUE(Value) \
            NameValues.Add(#Value, Value);

#define ADD_SCOPED_ENUM_VALUE(Value) \
            NameValues.Add(#Value, (int32)EnumType::Value);

#define END_EXPORT_ENUM(Enum) \
        } \
    } Exported##Enum(#Enum);


#include "UnLuaEx.inl"
