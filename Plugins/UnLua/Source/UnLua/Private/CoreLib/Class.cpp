#include "lua.hpp"
#include "lstate.h"
#include "Class.h"
#include "LuaCore.h"
#include "ReflectionUtils/FieldDesc.h"
#include "ReflectionUtils/ReflectionRegistry.h"

extern int32 UObject_Identical(lua_State* L);
extern int32 UObject_Delete(lua_State* L);

FClassDesc* Class_CheckParam(lua_State* L)
{
    FClassDesc* ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if ((!ClassDesc)
        || (!GReflectionRegistry.IsDescValid(ClassDesc, DESC_CLASS)))
    {
        UE_LOG(LogUnLua, Log, TEXT("Class : Invalid FClassDesc!"));
        return NULL;
    }

    if (!ClassDesc->IsValid())
    {
        //UE_LOG(LogUnLua, Log, TEXT("Class : Try to release empty FClassDesc(Name : %s, Address : %p)!"),*ClassDesc->GetName(),ClassDesc);
        //GReflectionRegistry.UnRegisterClass(ClassDesc);
        return NULL;
    }

    UClass* Class = ClassDesc->AsClass();
    if (!Class)
    {
        UE_LOG(LogUnLua, Log, TEXT("Class : ClassDesc type is not class(Name : %s, Address : %p)"), *ClassDesc->GetName(), ClassDesc);
        return NULL;
    }

    return ClassDesc;
}

/**
 * Generic closure to get UClass for a type
 */
int32 Class_StaticClass(lua_State* L)
{
    FClassDesc* ClassDesc = Class_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UClass* Class = ClassDesc->AsClass();
    UnLua::PushUObject(L, Class);
    return 1;
}

/**
 * Cast a UObject
 */
int32 Class_Cast(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject* Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        return 0;
    }

    UClass* Class = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (Class && (Object->IsA(Class) || (Class->HasAnyClassFlags(CLASS_Interface) && Class != UInterface::StaticClass() && Object->GetClass()->ImplementsInterface(Class))))
    {
        lua_pushvalue(L, 1);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

void UnLua::CoreLib::RegisterClass(lua_State* L, FClassDesc* InClass)
{
    lua_pushstring(L, "ClassDesc"); // Key
    lua_pushlightuserdata(L, InClass); // FClassDesc
    lua_rawset(L, -3);

    lua_pushstring(L, "StaticClass"); // Key
    lua_pushlightuserdata(L, InClass); // FClassDesc
    lua_pushcclosure(L, Class_StaticClass, 1); // closure
    lua_rawset(L, -3);

    lua_pushstring(L, "Cast"); // Key
    lua_pushcfunction(L, Class_Cast); // C function
    lua_rawset(L, -3);

    lua_pushstring(L, "__eq"); // Key
    lua_pushcfunction(L, UObject_Identical); // C function
    lua_rawset(L, -3);

    lua_pushstring(L, "__gc"); // Key
    lua_pushcfunction(L, UObject_Delete); // C function
    lua_rawset(L, -3);
}
