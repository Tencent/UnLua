#include "ScriptStruct.h"
#include "lua.hpp"
#include "LuaCore.h"
#include "UEObjectReferencer.h"
#include "ReflectionUtils/ClassDesc.h"
#include "ReflectionUtils/ReflectionRegistry.h"

FClassDesc* ScriptStruct_CheckParam(lua_State* L)
{
    FClassDesc* ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if ((!ClassDesc)
        || (!GReflectionRegistry.IsDescValid(ClassDesc, DESC_CLASS)))
    {
        UE_LOG(LogUnLua, Log, TEXT("ScriptStruct : Invalid FClassDesc!"));
        return NULL;
    }
    if (!ClassDesc->IsValid())
    {
        //UE_LOG(LogUnLua, Log, TEXT("ScriptStruct : Try to release empty FClassDesc(Name : %s, Address : %p)!"),*ClassDesc->GetName(),ClassDesc);
        //GReflectionRegistry.UnRegisterClass(ClassDesc);
        return NULL;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UE_LOG(LogUnLua, Log, TEXT("ScriptStruct : ClassDesc type is not script struct(Name : %s, Address : %p)"), *ClassDesc->GetName(), ClassDesc);
        return NULL;
    }

    return ClassDesc;
}

/**
 * Generic closure to create a UScriptStruct instance
 */
int32 ScriptStruct_New(lua_State* L)
{
    FClassDesc* ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();
    void* Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), TCHAR_TO_UTF8(*ClassDesc->GetName()), ClassDesc->GetUserdataPadding());
    ScriptStruct->InitializeStruct(Userdata);

    return 1;
}

/**
 * Generic GC function for UScriptStruct
 */
int32 ScriptStruct_Delete(lua_State* L)
{
    FClassDesc* ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();

    bool bTwoLvlPtr = false;
    void* Userdata = GetUserdataFast(L, 1, &bTwoLvlPtr);
    if (Userdata)
    {
        //struct in userdata memory
        if (!bTwoLvlPtr)
        {
            if (!(ScriptStruct->StructFlags & (STRUCT_IsPlainOldData | STRUCT_NoDestructor)))
            {
                ScriptStruct->DestroyStruct(Userdata);
            }
        }

        ClassDesc->SubRef();

#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogTemp, Log, TEXT("ScriptStruct_Delete : %s"), *ClassDesc->GetName());
#endif
        GReflectionRegistry.TryUnRegisterClass(ClassDesc);
    }
    else
    {
        if (!ScriptStruct->IsNative())
        {
            GObjectReferencer.RemoveObjectRef(ScriptStruct);
        }

        GReflectionRegistry.UnRegisterClass(ClassDesc);
    }
    return 0;
}

/**
 * Generic closure to copy a UScriptStruct
 */
int32 ScriptStruct_CopyFrom(lua_State* L)
{
    FClassDesc* ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();

    void* Src = GetCppInstanceFast(L, 1);
    void* Userdata = nullptr;
    if (lua_gettop(L) > 1)
    {
        Userdata = GetCppInstanceFast(L, 2);
        lua_pushvalue(L, 2);
    }
    else
    {
        Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), TCHAR_TO_UTF8(*ClassDesc->GetName()), ClassDesc->GetUserdataPadding());
        ScriptStruct->InitializeStruct(Userdata);
    }
    ScriptStruct->CopyScriptStruct(Src, Userdata);
    return 1;
}

/**
 * Generic closure to copy a UScriptStruct
 */
int32 ScriptStruct_Copy(lua_State* L)
{
    FClassDesc* ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();

    void* Src = GetCppInstanceFast(L, 1);
    void* Userdata = nullptr;
    if (lua_gettop(L) > 1)
    {
        Userdata = GetCppInstanceFast(L, 2);
        lua_pushvalue(L, 2);
    }
    else
    {
        Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), TCHAR_TO_UTF8(*ClassDesc->GetName()), ClassDesc->GetUserdataPadding());
        ScriptStruct->InitializeStruct(Userdata);
    }
    ScriptStruct->CopyScriptStruct(Userdata, Src);
    return 1;
}

/**
 * Generic closure to compare two UScriptStructs
 */
int32 ScriptStruct_Compare(lua_State* L)
{
    FClassDesc* ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();

    void* A = GetCppInstanceFast(L, 1);
    void* B = GetCppInstanceFast(L, 2);
    bool bResult = A && B ? ScriptStruct->CompareScriptStruct(A, B, /*PPF_None*/0) : false;
    lua_pushboolean(L, bResult);
    return 1;
}

void UnLua::CoreLib::RegisterScriptStruct(lua_State* L, FClassDesc* InClass)
{
    lua_pushlightuserdata(L, InClass); // FClassDesc

    lua_pushstring(L, "Copy"); // Key
    lua_pushvalue(L, -2); // FClassDesc
    lua_pushcclosure(L, ScriptStruct_Copy, 1); // closure
    lua_rawset(L, -4);

    lua_pushstring(L, "CopyFrom"); // Key
    lua_pushvalue(L, -2); // FClassDesc
    lua_pushcclosure(L, ScriptStruct_CopyFrom, 1); // closure
    lua_rawset(L, -4);

    lua_pushstring(L, "__eq"); // Key
    lua_pushvalue(L, -2); // FClassDesc
    lua_pushcclosure(L, ScriptStruct_Compare, 1); // closure
    lua_rawset(L, -4);

    //if (!(ScriptStruct->StructFlags & (STRUCT_IsPlainOldData | STRUCT_NoDestructor)))
    {
        lua_pushstring(L, "__gc"); // Key
        lua_pushvalue(L, -2); // FClassDesc
        lua_pushcclosure(L, ScriptStruct_Delete, 1); // closure
        lua_rawset(L, -4);
    }

    lua_pushstring(L, "__call"); // Key
    lua_pushvalue(L, -2); // FClassDesc
    lua_pushcclosure(L, ScriptStruct_New, 1); // closure
    lua_rawset(L, -4);

    lua_pop(L, 1);
}
