#include "Struct.h"
#include "lua.hpp"
#include "LuaCore.h"
#include "ReflectionUtils/FieldDesc.h"
#include "ReflectionUtils/ReflectionRegistry.h"

/**
 * Generic closure to call a UFunction
 */
int32 Class_CallUFunction(lua_State* L)
{
    //!!!Fix!!!
    //delete desc when is not valid
    FFunctionDesc* Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!GReflectionRegistry.IsDescValidWithObjectCheck(Function, DESC_FUNCTION))
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid function descriptor! %p"), ANSI_TO_TCHAR(__FUNCTION__), Function);
        return 0;
    }
    int32 NumParams = lua_gettop(L);
    int32 NumResults = Function->CallUE(L, NumParams);
    return NumResults;
}

/**
 * Generic closure to call a latent function
 */
int32 Class_CallLatentFunction(lua_State* L)
{
    FFunctionDesc* Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!GReflectionRegistry.IsDescValidWithObjectCheck(Function, DESC_FUNCTION))
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid function descriptor!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    int32 ThreadRef = GLuaCxt->FindThread(L);
    if (ThreadRef == LUA_REFNIL)
    {
        int32 Value = lua_pushthread(L);
        if (Value == 1)
        {
            lua_pop(L, 1);
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Can't call latent action in main lua thread!"), ANSI_TO_TCHAR(__FUNCTION__));
            return 0;
        }

        ThreadRef = luaL_ref(L, LUA_REGISTRYINDEX);
        GLuaCxt->AddThread(L, ThreadRef);
    }

    int32 NumParams = lua_gettop(L);
    int32 NumResults = Function->CallUE(L, NumParams, &ThreadRef);
    return lua_yield(L, NumResults);
}

/**
 * Push a field (property or function)
 */
static void PushField(lua_State* L, FFieldDesc* Field)
{
    check(Field && Field->IsValid());
    if (Field->IsProperty())
    {
        FPropertyDesc* Property = Field->AsProperty();
        lua_pushlightuserdata(L, Property); // Property
    }
    else
    {
        FFunctionDesc* Function = Field->AsFunction();
        lua_pushlightuserdata(L, Function); // Function
        if (Function->IsLatentFunction())
        {
            lua_pushcclosure(L, Class_CallLatentFunction, 1); // closure
        }
        else
        {
            lua_pushcclosure(L, Class_CallUFunction, 1); // closure
        }
    }
}

/**
 * Get a field (property or function)
 */
static int32 GetField(lua_State* L)
{
    int32 Type = lua_getmetatable(L, 1); // get meta table of table/userdata (first parameter passed in)
    check(Type == 1 && lua_istable(L, -1));

    lua_pushvalue(L, 2); // push key
    Type = lua_rawget(L, -2);

    if (Type == LUA_TNIL)
    {
        lua_pop(L, 1);

        lua_pushstring(L, "__name");
        Type = lua_rawget(L, -2);
        check(Type == LUA_TSTRING);

        const char* ClassName = lua_tostring(L, -1);
        const char* FieldName = lua_tostring(L, 2);
        lua_pop(L, 1);

        // desc maybe released on c++ side,but lua side may still hold it
        FClassDesc* ClassDesc = GReflectionRegistry.FindClass(ClassName);
        if (!ClassDesc)
        {
            lua_pushnil(L);
        }
        else
        {
            FScopedSafeClass SafeClass(ClassDesc);
            FFieldDesc* Field = ClassDesc->RegisterField(FieldName, ClassDesc);
            if (Field && Field->IsValid())
            {
                bool bCached = false;
                bool bInherited = Field->IsInherited();
                if (bInherited)
                {
                    FString SuperStructName = Field->GetOuterName();
                    Type = luaL_getmetatable(L, TCHAR_TO_UTF8(*SuperStructName));
                    check(Type == LUA_TTABLE);
                    lua_pushvalue(L, 2);
                    Type = lua_rawget(L, -2);
                    bCached = Type != LUA_TNIL;
                    if (!bCached)
                    {
                        lua_pop(L, 1);
                    }
                }

                if (!bCached)
                {
                    PushField(L, Field); // Property / closure
                    lua_pushvalue(L, 2); // key
                    lua_pushvalue(L, -2); // Property / closure
                    lua_rawset(L, -4);
                }
                if (bInherited)
                {
                    lua_remove(L, -2);
                    lua_pushvalue(L, 2); // key
                    lua_pushvalue(L, -2); // Property / closure
                    lua_rawset(L, -4);
                }
            }
            else
            {
                if (ClassDesc->IsClass())
                {
                    luaL_getmetatable(L, "UClass");
                    lua_pushvalue(L, 2); // push key
                    lua_rawget(L, -2);
                    lua_remove(L, -2);
                }
                else
                {
                    lua_pushnil(L);
                }
            }
        }
    }
    lua_remove(L, -2);
    return 1;
}

/**
 * __index meta methods for class
 */
int32 Class_Index(lua_State* L)
{
    GetField(L);
    if (lua_islightuserdata(L, -1))
    {
        bool bValid = false;
        UnLua::ITypeOps* Property = (UnLua::ITypeOps*)lua_touserdata(L, -1);
        if (Property)
        {
            if (GReflectionRegistry.IsDescValidWithObjectCheck(Property, DESC_PROPERTY))
            {
                bValid = true;
            }

            if ((!bValid)
                && (Property->StaticExported))
            {
                bValid = true;
            }

            void* ContainerPtr = GetCppInstance(L, 1);

            if ((bValid)
                && (ContainerPtr))
            {
                Property->Read(L, ContainerPtr, false);
                lua_remove(L, -2);
            }
        }
        else
        {
            lua_pushnil(L);
            lua_remove(L, -2);
        }
    }
    return 1;
}

bool IsPropertyOwnerTypeValid(UnLua::ITypeOps* InProperty, void* InContainerPtr)
{
    if (InProperty->StaticExported)
        return true;

    UnLua::ITypeInterface* TypeInterface = (UnLua::ITypeInterface*)InProperty;
    FProperty* Property = TypeInterface->GetUProperty();
    if (!Property)
        return true;

    UObject* Object = (UObject*)InContainerPtr;
    UClass* OwnerClass = Property->GetOwnerClass();
    if (!OwnerClass)
        return true;

    if (Object->IsA(OwnerClass))
        return true;

    UE_LOG(LogUnLua, Error, TEXT("Writing property to invalid owner. %s should be a %s."), *Object->GetName(), *OwnerClass->GetName());
    return false;
}

/**
 * __newindex meta methods for class
 */
int32 Class_NewIndex(lua_State* L)
{
    GetField(L);
    if (lua_islightuserdata(L, -1))
    {
        UnLua::ITypeOps* Property = (UnLua::ITypeOps*)lua_touserdata(L, -1);
        if (Property)
        {
            const bool bValid = GReflectionRegistry.IsDescValidWithObjectCheck(Property, DESC_PROPERTY) || Property->StaticExported;
            void* ContainerPtr = GetCppInstance(L, 1);
            if (bValid && ContainerPtr)
            {
#if ENABLE_TYPE_CHECK
                if (IsPropertyOwnerTypeValid(Property, ContainerPtr))
                    Property->Write(L, ContainerPtr, 3);
#else
                Property->Write(L, ContainerPtr, 3);
#endif
            }
        }
    }
    else
    {
        int32 Type = lua_type(L, 1);
        if (Type == LUA_TTABLE)
        {
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_rawset(L, 1);

            //UE_LOG(LogUnLua, Warning, TEXT("%s: You are modifying metatable! Please make sure you know what you are doing!"), ANSI_TO_TCHAR(__FUNCTION__));
        }
    }
    lua_pop(L, 1);
    return 0;
}

void UnLua::CoreLib::RegisterStruct(lua_State* L, FClassDesc* InStruct)
{
    lua_pushstring(L, "__index"); // 2
    lua_pushcfunction(L, Class_Index); // 3
    lua_rawset(L, -3);

    lua_pushstring(L, "__newindex"); // 2
    lua_pushcfunction(L, Class_NewIndex); // 3
    lua_rawset(L, -3);
    uint64 TypeHash = (uint64)InStruct->AsStruct();
    lua_pushstring(L, "TypeHash");
    lua_pushnumber(L, TypeHash);
    lua_rawset(L, -3);
}
