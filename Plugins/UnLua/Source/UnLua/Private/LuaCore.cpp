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

#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "LuaContext.h"
#include "UnLua.h"
#include "UnLuaDelegates.h"
#include "UEObjectReferencer.h"
#include "CollisionHelper.h"
#include "DelegateHelper.h"
#include "Containers/LuaSet.h"
#include "Containers/LuaMap.h"
#include "ReflectionUtils/FieldDesc.h"
#include "ReflectionUtils/PropertyCreator.h"
#include "ReflectionUtils/ReflectionRegistry.h"
#include "Kismet/KismetSystemLibrary.h"

extern "C"
{
#include "lfunc.h"
#include "lstate.h"
#include "lobject.h"
}

const FScriptContainerDesc FScriptContainerDesc::Array(sizeof(FLuaArray), "TArray");
const FScriptContainerDesc FScriptContainerDesc::Set(sizeof(FLuaSet), "TSet");
const FScriptContainerDesc FScriptContainerDesc::Map(sizeof(FLuaMap), "TMap");

/**
 * Global __index meta method
 */
static int32 UE4_Index(lua_State *L)
{
    int32 Type = lua_type(L, 2);
    if (Type == LUA_TSTRING)
    {
        const char *Name = lua_tostring(L, 2);
        const char Prefix = Name[0];
        if (Prefix == 'U' || Prefix == 'A' || Prefix == 'F')
        {
            RegisterClass(L, Name);
        }
        else if (Prefix == 'E')
        {
            RegisterEnum(L, Name);
        }
    }
    lua_rawget(L, 1);
    return 1;
}

/**
 * Create 'UE4' namespace (a Lua table)
 */
void CreateNamespaceForUE(lua_State *L)
{
#if WITH_UE4_NAMESPACE
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, UE4_Index);
    lua_rawset(L, -3);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "UE4");

    lua_pushboolean(L, true);
#else
    lua_pushboolean(L, false);
#endif
    lua_setglobal(L, "WITH_UE4_NAMESPACE");
}

/**
 * Set the name for a Lua table which on the top of the stack
 */
void SetTableForClass(lua_State *L, const char *Name)
{
#if WITH_UE4_NAMESPACE
    lua_getglobal(L, "UE4");
    lua_pushstring(L, Name);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 2);
#else
    lua_setglobal(L, Name);
#endif
}

/**
 * Get 'TValue' from Lua stack
 */
static TValue* GetTValue(lua_State *L, int32 Index)
{
    CallInfo *ci = L->ci;
    if (Index > 0)
    {
        TValue *V = ci->func + Index;
        checkf(Index <= ci->top - (ci->func + 1), TEXT("LUA STACK OVERFLOW!!! Maybe there are bugs in your codes, or use lua_checkstack() to make sure there is enough space on the lua stack!"));
        return V >= L->top ? (TValue*)NULL : V;
    }
    else if (Index > LUA_REGISTRYINDEX)             // negative
    {
        check(Index != 0 && -Index <= L->top - (ci->func + 1));
        return L->top + Index;
    }
    else if (Index == LUA_REGISTRYINDEX)
    {
        return &G(L)->l_registry;
    }
    else                                            // upvalues
    {
        Index = LUA_REGISTRYINDEX - Index;
        check(Index <= MAXUPVAL + 1);
        if (ttislcf(ci->func))
        {
            return (TValue*)NULL;                   // light C function has no upvalues
        }
        else
        {
            CClosure *Closure = clCvalue(ci->func);
            return (Index <= Closure->nupvalues) ? &Closure->upvalue[Index - 1] : (TValue*)NULL;
        }
    }
}

/**
 * Calculate padding size for userdata
 */
uint8 CalcUserdataPadding(int32 Alignment)
{
    return (uint8)(Align(sizeof(UUdata), Alignment) - sizeof(UUdata));      // sizeof(UUdata) == 40
}

#define BIT_VARIANT_TAG            (1 << 7)         // variant tag for userdata
#define BIT_TWOLEVEL_PTR        (1 << 5)            // two level pointer flag
#define BIT_SCRIPT_CONTAINER    (1 << 4)            // script container (TArray, TSet, TMap) flag

/**
 * Set padding size info for userdata
 */
void SetUserdataPadding(lua_State *L, int32 Index, uint8 PaddingBytes)
{
    TValue *Value = GetTValue(L, Index);
    if ((Value->tt_ & 0x0F) == LUA_TUSERDATA && PaddingBytes < 128)
    {
        Udata *U = uvalue(Value);
        U->ttuv_ |= BIT_VARIANT_TAG;
        U->user_.b = (int32)PaddingBytes;
    }
}

/**
 * Set the userdata as a two level pointer
 */
void MarkUserdataTwoLvlPtr(lua_State *L, int32 Index)
{
    TValue *Value = GetTValue(L, Index);
    if ((Value->tt_ & 0x0F) == LUA_TUSERDATA)
    {
        Udata *U = uvalue(Value);
        U->ttuv_ |= (BIT_VARIANT_TAG | BIT_TWOLEVEL_PTR);
    }
}

/**
 * Get the address of userdata
 *
 * @param Index - Lua stack index
 * @param[out] OutTwoLvlPtr - whether the address is a two level pointer
 * @param[out] OutClassMetatable - whether the userdata comes from a metatable
 * @return - the untyped dynamic array
 */
void* GetUserdata(lua_State *L, int32 Index, bool *OutTwoLvlPtr, bool *OutClassMetatable)
{
    // Index < LUA_REGISTRYINDEX => upvalues
    if (Index < 0 && Index > LUA_REGISTRYINDEX)
    {
        int32 Top = lua_gettop(L);
        Index = Top + Index + 1;
    }

    void *Userdata = nullptr;
    bool bTwoLvlPtr = false, bClassMetatable = false;

    int32 Type = lua_type(L, Index);
    switch (Type)
    {
    case LUA_TTABLE:
        {
            lua_pushstring(L, "Object");
            Type = lua_rawget(L, Index);
            if (Type == LUA_TUSERDATA)
            {
                Userdata = lua_touserdata(L, -1);           // get the raw UObject
            }
            else
            {
                lua_pop(L, 1);
                lua_pushstring(L, "ClassDesc");
                Type = lua_rawget(L, Index);
                if (Type == LUA_TLIGHTUSERDATA)
                {
                    Userdata = lua_touserdata(L, -1);       // get the 'FClassDesc' pointer
                    bClassMetatable = true;
                }
            }
            bTwoLvlPtr = true;                              // set two level pointer flag
            lua_pop(L, 1);
        }
        break;
    case LUA_TUSERDATA:
        Userdata = GetUserdataFast(L, Index, &bTwoLvlPtr);  // get the userdata pointer
        break;
    }

    if (OutTwoLvlPtr)
    {
        *OutTwoLvlPtr = bTwoLvlPtr;
    }
    if (OutClassMetatable)
    {
        *OutClassMetatable = bClassMetatable;
    }

    return Userdata;
}

/**
 * Get the address of userdata, fast path
 */
void* GetUserdataFast(lua_State *L, int32 Index, bool *OutTwoLvlPtr)
{
    bool bTwoLvlPtr = false;
    void *Userdata = nullptr;

    TValue *Value = GetTValue(L, Index);
    int32 Type = Value->tt_ & 0x0F;
    if (Type == LUA_TUSERDATA)
    {
        Udata *U = uvalue(Value);
        uint8 *Buffer = (uint8*)U + sizeof(UUdata);
        if (U->ttuv_ & BIT_VARIANT_TAG)                             // if the userdata has a variant tag
        {
            bTwoLvlPtr = (U->ttuv_ & BIT_TWOLEVEL_PTR) != 0;        // test if the userdata is a two level pointer
            Userdata = bTwoLvlPtr ? Buffer : Buffer + (U->user_.b & 0x000000F8);    // add padding to userdata if it's not a two level pointer
        }
        else
        {
            Userdata = Buffer;
        }
    }
    else if (Type == LUA_TLIGHTUSERDATA)
    {
        Userdata = Value->value_.p;                                 // get the light userdata
    }

    if (OutTwoLvlPtr)
    {
        *OutTwoLvlPtr = bTwoLvlPtr;                                 // set two level pointer flag
    }

    return Userdata;
}

/**
 * Set metatable for the userdata/table on the top of the stack
 */
bool TryToSetMetatable(lua_State *L, const char *MetatableName)
{
    int32 Type = luaL_getmetatable(L, MetatableName);
    if (Type == LUA_TTABLE)
    {
        lua_setmetatable(L, -2);                                    // set the metatable directly
        return true;
    }
    else
    {
        lua_pop(L, 1);
        if (RegisterClass(L, MetatableName))                        // register class first
        {
            luaL_getmetatable(L, MetatableName);                    // get the metatable and push it on the top
            lua_setmetatable(L, -2);                                // set the metatable
            return true;
        }
    }
    return false;
}

/**
 * Create a new userdata with padding size
 */
void* NewUserdataWithPadding(lua_State *L, int32 Size, const char *MetatableName, uint8 PaddingSize)
{
    if (Size < 1)
    {
        // userdata size must be greater than 0
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid size!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }
    if ((PaddingSize & 0x07) != 0)          // 8 bytes padding at least..., 8, 24, 88
    {
        // padding size must be greater or equal to 8 bytes
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid padding size!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }

    void *Userdata = lua_newuserdata(L, Size + PaddingSize);        // userdata size must add padding size
    if (PaddingSize > 0)
    {
        SetUserdataPadding(L, -1, PaddingSize);                     // store padding size in internal userdata structure
    }
    if (MetatableName)
    {
        bool bSuccess = TryToSetMetatable(L, MetatableName);        // set metatable
        if (!bSuccess)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable, metatable name: !"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(MetatableName));
            return nullptr;
        }
    }
    return (uint8*)Userdata + PaddingSize;                          // return 'valid' address (userdata memory address + padding size)
}

/**
 * Get a cpp instance's address
 */
void* GetCppInstance(lua_State *L, int32 Index)
{
    bool bTwoLvlPtr = false;
    void *Userdata = GetUserdata(L, Index, &bTwoLvlPtr);
    if (Userdata)
    {
        return bTwoLvlPtr ? *((void**)Userdata) : Userdata;         // return instance's address
    }
    return nullptr;
}

/**
 * Get a cpp instance's address, fast path
 */
void* GetCppInstanceFast(lua_State *L, int32 Index)
{
    bool bTwoLvlPtr = false;
    void *Userdata = GetUserdataFast(L, Index, &bTwoLvlPtr);
    if (Userdata)
    {
        return bTwoLvlPtr ? *((void**)Userdata) : Userdata;         // return instance's address
    }
    return nullptr;
}

/**
 * Set script container flag for the userdata
 */
static void MarkScriptContainer(lua_State *L, int32 Index)
{
    TValue *Value = GetTValue(L, Index);
    if ((Value->tt_ & 0x0F) == LUA_TUSERDATA)
    {
        Udata *U = uvalue(Value);
        U->ttuv_ |= (BIT_VARIANT_TAG | BIT_SCRIPT_CONTAINER);       // set flags
    }
}

/**
 * Create a new userdata for a script container
 */
void* NewScriptContainer(lua_State *L, const FScriptContainerDesc &Desc)
{
    void *Userdata = lua_newuserdata(L, Desc.GetSize());
    luaL_setmetatable(L, Desc.GetName());   // set metatable
    MarkScriptContainer(L, -1);             // set flag
    return Userdata;
}

/**
 * Find a cached script container or create a new one
 *
 * @return - null if container is already cached, or the new created userdata otherwise
 */
void* CacheScriptContainer(lua_State *L, void *Key, const FScriptContainerDesc &Desc)
{
    if (!Key)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid key!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }

    // return null if container is already cached, or create/cache/return a new ud
    void *Userdata = nullptr;
    lua_getfield(L, LUA_REGISTRYINDEX, "ScriptContainerMap");
    lua_pushlightuserdata(L, Key);
    int32 Type = lua_rawget(L, -2);             
    if (Type == LUA_TNIL)
    {
        lua_pop(L, 1);

        Userdata = lua_newuserdata(L, Desc.GetSize());      // create new userdata
        luaL_setmetatable(L, Desc.GetName());               // set metatable
        lua_pushlightuserdata(L, Key);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);                                  // cache it in 'ScriptContainerMap'

        MarkScriptContainer(L, -1);                         // set flag
    }
#if UE_BUILD_DEBUG
    else
    {
        check(Type == LUA_TUSERDATA);
    }
#endif
    lua_remove(L, -2);
    return Userdata;            // return null if container is already cached, or the new created userdata otherwise
}

/**
 * Get a script container at the given stack index
 */
void* GetScriptContainer(lua_State *L, int32 Index)
{
    TValue *Value = GetTValue(L, Index);
    if ((Value->tt_ & 0x0F) == LUA_TUSERDATA)
    {
        Udata *U = uvalue(Value);
        uint8 *Userdata = (uint8*)U + sizeof(UUdata);
        uint8 Flag = (BIT_VARIANT_TAG | BIT_SCRIPT_CONTAINER);              // variant tags
        return (U->ttuv_ & Flag) == Flag ? *((void**)Userdata) : nullptr;
    }
    return nullptr;
}

/**
 * Get a script container according to a 'Key'
 */
void* GetScriptContainer(lua_State *L, void *Key)
{
    if (!Key)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid key!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }

    // find it in 'ScriptContainerMap'
    lua_getfield(L, LUA_REGISTRYINDEX, "ScriptContainerMap");
    lua_pushlightuserdata(L, Key);
    int32 Type = lua_rawget(L, -2);
    if (Type != LUA_TNIL)
    {
        lua_remove(L, -2);
        return *((void**)lua_touserdata(L, -1));
    }
    lua_pop(L, 2);
    return nullptr;
}

/**
 * Remove a cached script container from 'ScriptContainerMap'
 */
void RemoveCachedScriptContainer(lua_State *L, void *Key)
{
    if (!L || !Key)
    {
        return;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "ScriptContainerMap");
    lua_pushlightuserdata(L, Key);
    int32 Type = lua_rawget(L, -2);
    if (Type != LUA_TNIL)
    {
        lua_pushlightuserdata(L, Key);
        lua_pushnil(L);
        lua_rawset(L, -4);
    }
    lua_pop(L, 2);
}

/**
 * Push a UObject to Lua stack
 */
void PushObjectCore(lua_State *L, UObjectBaseUtility *Object)
{
    if (!Object)
    {
        lua_pushnil(L);
        return;
    }

    void **Dest = (void**)lua_newuserdata(L, sizeof(void*));        // create a userdata
    *Dest = Object;                                                 // store the UObject address
    MarkUserdataTwoLvlPtr(L, -1);                                   // set two level pointer flag

    UClass *Class = Object->GetClass();
    if (Class->IsChildOf(UScriptStruct::StaticClass()))
    {
        // the UObject is 'UScriptStruct'
        int32 Type = luaL_getmetatable(L, "UObject");
        check(Type != LUA_TNIL);
        lua_setmetatable(L, -2);
    }
    else
    {
        bool bSuccess = false;
        if (Class->IsChildOf(UClass::StaticClass()))
        {
            // the UObject is 'UClass'
            bSuccess = TryToSetMetatable(L, "UClass");
        }
        else
        {
            // the UObject is object instance
            TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ClassName(*FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName()));
            bSuccess = TryToSetMetatable(L, ClassName.Get());
        }
        if (!bSuccess)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable!"), ANSI_TO_TCHAR(__FUNCTION__));
        }
    }
}

/**
 * Push a integer
 */
static void PushIntegerElement(lua_State *L, FNumericProperty *Property, void *Value)
{
    lua_pushinteger(L, Property->GetUnsignedIntPropertyValue(Value));
}

/**
 * Push a float
 */
static void PushFloatElement(lua_State *L, FNumericProperty *Property, void *Value)
{
    lua_pushnumber(L, Property->GetFloatingPointPropertyValue(Value));
}

/**
 * Push a enum
 */
static void PushEnumElement(lua_State *L, FNumericProperty *Property, void *Value)
{
    lua_pushinteger(L, Property->GetSignedIntPropertyValue(Value));
}

/**
 * Push a FName
 */
static void PushFNameElement(lua_State *L, FNameProperty *Property, void *Value)
{
    lua_pushstring(L, TCHAR_TO_UTF8(*Property->GetPropertyValue(Value).ToString()));
}

/**
 * Push a FString
 */
static void PushFStringElement(lua_State *L, FStrProperty *Property, void *Value)
{
    lua_pushstring(L, TCHAR_TO_UTF8(*Property->GetPropertyValue(Value)));
}

/**
 * Push a FText
 */
static void PushFTextElement(lua_State *L, FTextProperty *Property, void *Value)
{
    lua_pushstring(L, TCHAR_TO_UTF8(*Property->GetPropertyValue(Value).ToString()));
}

/**
 * Push a UObject
 */
static void PushObjectElement(lua_State *L, FObjectPropertyBase *Property, void *Value)
{
    UObject *Object = Property->GetObjectPropertyValue(Value);
    GObjectReferencer.AddObjectRef(Object);
    PushObjectCore(L, Object);
}

/**
 * Push a Interface
 */
static void PushInterfaceElement(lua_State *L, FInterfaceProperty *Property, void *Value)
{
    const FScriptInterface &Interface = Property->GetPropertyValue(Value);
    UObject *Object = Interface.GetObject();
    GObjectReferencer.AddObjectRef(Object);
    PushObjectCore(L, Object);
}

/**
 * Push a ScriptStruct
 */
static void PushStructElement(lua_State *L, FProperty *Property, void *Value)
{
    void **Userdata = (void**)lua_newuserdata(L, sizeof(void*));
    *Userdata = Value;
    MarkUserdataTwoLvlPtr(L, -1);
}

/**
 * Push a delegate
 */
static void PushDelegateElement(lua_State *L, FDelegateProperty *Property, void *Value)
{
    FScriptDelegate *ScriptDelegate = Property->GetPropertyValuePtr(Value);
    FDelegateHelper::PreBind(ScriptDelegate, Property);
    void **Userdata = (void**)lua_newuserdata(L, sizeof(void*));
    *Userdata = ScriptDelegate;
    MarkUserdataTwoLvlPtr(L, -1);
}

/**
 * Push a multicast delegate
 */
static void PushMCDelegateElement(lua_State *L, FMulticastDelegateProperty *Property, void *Value)
{
#if ENGINE_MINOR_VERSION < 23
    FMulticastScriptDelegate *ScriptDelegate = Property->GetPropertyValuePtr(Value);
#else
    void *ScriptDelegate = Value;
#endif
    FDelegateHelper::PreAdd(ScriptDelegate, Property);
    void **Userdata = (void**)lua_newuserdata(L, sizeof(void*));
    *Userdata = ScriptDelegate;
    MarkUserdataTwoLvlPtr(L, -1);
}

template <typename T, bool WithMetaTableName>
class TPropertyArrayPushPolicy
{
public:
    static bool CheckMetaTable(const char *MetatableName) { return true; }
    static void PrePushArray(lua_State *L, const char *MetatableName) {}
    static void PostPushArray(lua_State *L) {}
    static void PostPushSingleElement(lua_State *L) { lua_rawset(L, -3); }
};

template <typename T>
class TPropertyArrayPushPolicy<T, true>
{
public:
    static bool CheckMetaTable(const char *MetatableName) { return MetatableName != nullptr; }
    static void PrePushArray(lua_State *L, const char *MetatableName) { luaL_getmetatable(L, MetatableName); }
    static void PostPushArray(lua_State *L) { lua_pop(L, 1); }

    static void PostPushSingleElement(lua_State *L)
    {
        lua_pushvalue(L, -3);
        lua_setmetatable(L, -2);
        lua_rawset(L, -4);
    }
};

/**
 * Push static property array
 */
template <typename T, bool WithMetaTableName>
static void PushPropertyArray(lua_State *L, T *Property, void *Value, void(*PushFunc)(lua_State*, T*, void*), const char *MetatableName = nullptr)
{
#if !UE_BUILD_SHIPPING
    if (!Property || !Value || Property->ArrayDim < 2 || Property->ElementSize < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }
#endif

    if (!TPropertyArrayPushPolicy<T, WithMetaTableName>::CheckMetaTable(MetatableName))
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "ArrayMap");         // get weak table 'ArrayMap'
    lua_pushlightuserdata(L, Value);
    int32 Type = lua_rawget(L, -2);
    if (Type != LUA_TTABLE)
    {
        check(Type == LUA_TNIL);
        lua_pop(L, 1);

        uint8 *ElementPtr = (uint8*)Value;
        lua_newtable(L);                                    // create a Lua table
        TPropertyArrayPushPolicy<T, WithMetaTableName>::PrePushArray(L, MetatableName);
        for (int32 i = 0; i < Property->ArrayDim; ++i)
        {
            lua_pushinteger(L, i + 1);
            PushFunc(L, Property, ElementPtr);
            ElementPtr += Property->ElementSize;
            TPropertyArrayPushPolicy<T, WithMetaTableName>::PostPushSingleElement(L);
        }
        TPropertyArrayPushPolicy<T, WithMetaTableName>::PostPushArray(L);

        lua_pushlightuserdata(L, Value);                    // cache the Lua table in 'ArrayMap'
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }
    lua_remove(L, -2);
}


void PushIntegerArray(lua_State *L, FNumericProperty *Property, void *Value)
{
    PushPropertyArray<FNumericProperty, false>(L, Property, Value, PushIntegerElement);
}

void PushFloatArray(lua_State *L, FNumericProperty *Property, void *Value)
{
    PushPropertyArray<FNumericProperty, false>(L, Property, Value, PushFloatElement);
}

void PushEnumArray(lua_State *L, FNumericProperty *Property, void *Value)
{
    PushPropertyArray<FNumericProperty, false>(L, Property, Value, PushEnumElement);
}

void PushFNameArray(lua_State *L, FNameProperty *Property, void *Value)
{
    PushPropertyArray<FNameProperty, false>(L, Property, Value, PushFNameElement);
}

void PushFStringArray(lua_State *L, FStrProperty *Property, void *Value)
{
    PushPropertyArray<FStrProperty, false>(L, Property, Value, PushFStringElement);
}

void PushFTextArray(lua_State *L, FTextProperty *Property, void *Value)
{
    PushPropertyArray<FTextProperty, false>(L, Property, Value, PushFTextElement);
}

void PushObjectArray(lua_State *L, FObjectPropertyBase *Property, void *Value)
{
    PushPropertyArray<FObjectPropertyBase, false>(L, Property, Value, PushObjectElement);
}

void PushInterfaceArray(lua_State *L, FInterfaceProperty *Property, void *Value)
{
    PushPropertyArray<FInterfaceProperty, false>(L, Property, Value, PushInterfaceElement);
}

void PushDelegateArray(lua_State *L, FDelegateProperty *Property, void *Value)
{
    PushPropertyArray<FDelegateProperty, true>(L, Property, Value, PushDelegateElement, "FScriptDelegate");
}

void PushMCDelegateArray(lua_State *L, FMulticastDelegateProperty *Property, void *Value, const char *MetatableName)
{
    PushPropertyArray<FMulticastDelegateProperty, true>(L, Property, Value, PushMCDelegateElement, MetatableName);
}

void PushStructArray(lua_State *L, FProperty *Property, void *Value, const char *MetatableName)
{
    PushPropertyArray<FProperty, true>(L, Property, Value, PushStructElement, MetatableName);
}

/**
 * Create a Lua instance (table) for a UObject
 */
int32 NewLuaObject(lua_State *L, UObjectBaseUtility *Object, UClass *Class, const char *ModuleName)
{
    check(Object);

    lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");
    lua_pushlightuserdata(L, Object);
    lua_newtable(L);                                            // create a Lua table ('INSTANCE')
    PushObjectCore(L, Object);                                  // push UObject ('RAW_UOBJECT')
    lua_pushstring(L, "Object");
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);                                          // INSTANCET.Object = RAW_UOBJECT
    int32 Type = GetLoadedModule(L, ModuleName);                // push the required module/table ('REQUIRED_MODULE') to the top of the stack
    if (Class)
    {
        TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ClassName(*FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName()));
        Type = luaL_getmetatable(L, ClassName.Get());
        check(Type == LUA_TTABLE);
    }
    else
    {
        lua_getmetatable(L, -2);                                // get the metatable ('METATABLE_UOBJECT') of 'RAW_UOBJECT' 
    }
#if ENABLE_CALL_OVERRIDDEN_FUNCTION
    lua_pushstring(L, "Overridden");
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
#endif
    lua_setmetatable(L, -2);                                    // REQUIRED_MODULE.metatable = METATABLE_UOBJECT
    lua_setmetatable(L, -3);                                    // INSTANCE.metatable = REQUIRED_MODULE
    lua_pop(L, 1);
    lua_pushvalue(L, -1);
    int32 ObjectRef = luaL_ref(L, LUA_REGISTRYINDEX);           // keep a reference for 'INSTANCE'

    FUnLuaDelegates::OnObjectBinded.Broadcast(Object);          // 'INSTANCE' is on the top of stack now

    lua_rawset(L, -3);
    lua_pop(L, 1);
    return ObjectRef;
}

/**
 * Delete the Lua instance (table) for a UObject
 */
void DeleteLuaObject(lua_State *L, UObjectBaseUtility *Object)
{
    if (!Object)
    {
        return;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");            // get the object instance from 'ObjectMap'
    lua_pushlightuserdata(L, Object);
    int32 Type = lua_rawget(L, -2);
    if (Type == LUA_TTABLE || Type == LUA_TUSERDATA)
    {
        FUnLuaDelegates::OnObjectUnbinded.Broadcast(Object);    // object instance ('INSTANCE') is on the top of stack now

        // todo: add comments here...
        if (Type == LUA_TTABLE)
        {
            lua_pushstring(L, "Object");
            Type = lua_rawget(L, -2);
            check(Type == LUA_TUSERDATA);
            void *Userdata = lua_touserdata(L, -1);
            *((void**)Userdata) = nullptr;
            lua_pop(L, 2);
        }
        else
        {
            void *Userdata = lua_touserdata(L, -1);
            *((void**)Userdata) = nullptr;
            lua_pop(L, 1);
        }

        // INSTANCE.Object = nil
        lua_pushlightuserdata(L, Object);
        lua_pushnil(L);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }
    else
    {
        check(Type == LUA_TNIL);
        lua_pop(L, 2);
    }
}

/**
 * Get target UObject and Lua function pointer for a delegate
 */
int32 GetDelegateInfo(lua_State *L, int32 Index, UObject* &Object, const void* &Function)
{
    int32 Type = lua_type(L, Index);
    if (Type != LUA_TTABLE)
    {
        return INDEX_NONE;
    }

    Object = nullptr;
    Function = nullptr;
    int32 FuncIdxInTable = INDEX_NONE;
    for (int32 i = 1; i <= 2; ++i)
    {
        Type = lua_rawgeti(L, Index, i);
        if (Type == LUA_TFUNCTION)
        {
            Function = lua_topointer(L, -1);            // Lua function pointer
            FuncIdxInTable = i;
        }
        else
        {
            Object = UnLua::GetUObject(L, -1);          // target UObject
        }
        lua_pop(L, 1);
    }
    return !Object || !Function ? INDEX_NONE : FuncIdxInTable;
}

/**
 * Callback function to get function name 
 */
static bool GetFunctionName(lua_State *L, void *Userdata)
{
    int32 ValueType = lua_type(L, -1);
    if (ValueType == LUA_TFUNCTION)
    {
        TSet<FName> *FunctionNames = (TSet<FName>*)Userdata;
#if SUPPORTS_RPC_CALL
        FString FuncName(lua_tostring(L, -2));
        if (FuncName.EndsWith(TEXT("_RPC")))
        {
            FuncName = FuncName.Left(FuncName.Len() - 4);
        }
        FunctionNames->Add(FName(*FuncName));
#else
        FunctionNames->Add(FName(lua_tostring(L, -2)));
#endif
    }
    return true;
}

/**
 * Get all Lua function names defined in a required module/table
 */
bool GetFunctionList(lua_State *L, const char *InModuleName, TSet<FName> &FunctionNames)
{
    int32 Type = GetLoadedModule(L, InModuleName);
    if (Type == LUA_TNIL)
    {
        return false;
    }

    int32 N = 1;
    bool bNext = false;
    do 
    {
        bNext = TraverseTable(L, -1, &FunctionNames, GetFunctionName) > INDEX_NONE;
        if (bNext)
        {
            lua_pushstring(L, "Super");
            lua_rawget(L, -2);
            ++N;
            bNext = lua_istable(L, -1);
        }
    } while (bNext);
    lua_pop(L, N);
    return true;
}

/**
 * Get Lua instance for a UObject
 */
bool GetObjectMapping(lua_State *L, UObjectBaseUtility *Object)
{
    if (!Object)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid object!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");
    lua_pushlightuserdata(L, Object);
    int32 Type = lua_rawget(L, -2);
    if (Type != LUA_TNIL)
    {
        lua_remove(L, -2);
        return true;
    }
    lua_pop(L, 2);
    return false;
}

/**
 * Push a Lua function (by a function name) and push a UObject instance as its first parameter
 */
int32 PushFunction(lua_State *L, UObjectBaseUtility *Object, const char *FunctionName)
{
    int32 N = lua_gettop(L);
    lua_pushcfunction(L, UnLua::ReportLuaCallError);
    bool bSuccess = GetObjectMapping(L, Object);
    if (bSuccess)
    {
        int32 Type = lua_type(L, -1);
        if (Type == LUA_TTABLE /*|| Type == LUA_TUSERDATA*/)
        {
            if (lua_getmetatable(L, -1) == 1)
            {
                do
                {
                    lua_pushstring(L, FunctionName);
                    lua_rawget(L, -2);
                    if (lua_isfunction(L, -1))
                    {
                        lua_pushvalue(L, -3);
                        lua_remove(L, -3);
                        lua_remove(L, -3);
                        lua_pushvalue(L, -2);
                        return luaL_ref(L, LUA_REGISTRYINDEX);
                    }
                    else
                    {
                        lua_pop(L, 1);
                        lua_pushstring(L, "Super");
                        lua_rawget(L, -2);
                        lua_remove(L, -2);
                    }
                } while (lua_istable(L, -1));
            }
        }
    }
    if (int32 NumToPop = lua_gettop(L) - N)
    {
        lua_pop(L, NumToPop);
    }
    return INDEX_NONE;
}

/**
 * Push a Lua function (by a function reference) and push a UObject instance as its first parameter
 */
bool PushFunction(lua_State *L, UObjectBaseUtility *Object, int32 FunctionRef)
{
    int32 N = lua_gettop(L);
    lua_pushcfunction(L, UnLua::ReportLuaCallError);
    int32 Type = lua_rawgeti(L, LUA_REGISTRYINDEX, FunctionRef);
    if (Type == LUA_TFUNCTION)
    {
        UnLua::PushUObject(L, Object);
        return true;
    }
    if (int32 NumToPop = lua_gettop(L) - N)
    {
        lua_pop(L, NumToPop);
    }
    return false;
}

/**
 * Call a Lua function
 */
bool CallFunction(lua_State *L, int32 NumArgs, int32 NumResults)
{
    int32 ErrorReporterIdx = lua_gettop(L) - NumArgs - 1;
    int32 Code = lua_pcall(L, NumArgs, NumResults, -(NumArgs + 2));
    if (Code == LUA_OK)
    {
        lua_remove(L, ErrorReporterIdx);
        return true;
    }
    int32 TopIdx = lua_gettop(L);
    lua_pop(L, TopIdx - ErrorReporterIdx + 1);
    return false;
}

/**
 * Push a field (property or function)
 */
static void PushField(lua_State *L, FFieldDesc *Field)
{
    check(Field && Field->IsValid());
    if (Field->IsProperty())
    {
        FPropertyDesc *Property = Field->AsProperty();
        lua_pushlightuserdata(L, Property);                     // Property
    }
    else
    {
        FFunctionDesc *Function = Field->AsFunction();
        lua_pushlightuserdata(L, Function);                     // Function
        if (Function->IsLatentFunction())
        {
            lua_pushcclosure(L, Class_CallLatentFunction, 1);   // closure
        }
        else
        {
            lua_pushcclosure(L, Class_CallUFunction, 1);        // closure
        }
    }
}

/**
 * Get a field (property or function)
 */
static int32 GetField(lua_State *L)
{
    int32 n = lua_getmetatable(L, 1);       // get meta table of table/userdata (first parameter passed in)
    check(n == 1 && lua_istable(L, -1));
    lua_pushvalue(L, 2);                    // push key
    int32 Type = lua_rawget(L, -2);
    if (Type == LUA_TNIL)
    {
        lua_pop(L, 1);

        lua_pushstring(L, "__name");
        Type = lua_rawget(L, -2);
        check(Type == LUA_TSTRING);

        const char *ClassName = lua_tostring(L, -1);
        const char *FieldName = lua_tostring(L, 2);
        lua_pop(L, 1);

        FClassDesc *ClassDesc = GReflectionRegistry.FindClass(ClassName);
        check(ClassDesc);
        FScopedSafeClass SafeClass(ClassDesc);
        FFieldDesc *Field = ClassDesc->RegisterField(FieldName);
        if (Field && Field->IsValid())
        {
            bool bCached = false;
            bool bInherited = Field->IsInherited();
            if (bInherited)
            {
                FString SuperStructName = Field->GetOuterName();
                Type = luaL_getmetatable(L, TCHAR_TO_ANSI(*SuperStructName));
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
                PushField(L, Field);                // Property / closure
                lua_pushvalue(L, 2);                // key
                lua_pushvalue(L, -2);               // Property / closure
                lua_rawset(L, -4);
            }
            if (bInherited)
            {
                lua_remove(L, -2);
                lua_pushvalue(L, 2);                // key
                lua_pushvalue(L, -2);               // Property / closure
                lua_rawset(L, -4);
            }
        }
        else
        {
            if (ClassDesc->IsClass())
            {
                luaL_getmetatable(L, "UObject");
                lua_pushvalue(L, 2);                // push key
                Type = lua_rawget(L, -2);
#if WITH_EDITOR
                if (Type == LUA_TNIL)
                {
                    UE_LOG(LogUnLua, Verbose, TEXT("%s: Can't find %s in class %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(FieldName), ANSI_TO_TCHAR(ClassName));
                }
#endif
                lua_remove(L, -2);
            }
            else
            {
                lua_pushnil(L);
            }
        }
    }
    lua_remove(L, -2);
    return 1;
}

/**
 * Add a package path to package.path
 */
void AddPackagePath(lua_State *L, const char *Path)
{
    if (!Path)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s, Invalid package path!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    char FinalPath[MAX_SPRINTF];
    FCStringAnsi::Sprintf(FinalPath, "%s;%s", lua_tostring(L, -1), Path);
    lua_pushstring(L, FinalPath);
    lua_setfield(L, -3, "path");
    lua_pop(L, 2);
}

/**
 * package.loaded[ModuleName] = nil
 */
void ClearLoadedModule(lua_State *L, const char *ModuleName)
{
    if (!ModuleName)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s, Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return;
    }

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushnil(L);
    lua_setfield(L, -2, ModuleName);
    lua_pop(L, 2);
}

/**
 * Get package.loaded[ModuleName]
 */
int32 GetLoadedModule(lua_State *L, const char *ModuleName)
{
    if (!ModuleName)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s, Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return LUA_TNIL;
    }

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    int32 Type = lua_getfield(L, -1, ModuleName);
    lua_remove(L, -2);
    lua_remove(L, -2);
    return Type;
}

/**
 * Get collision related enums
 */
static bool RegisterCollisionEnum(lua_State *L, const char *Name, lua_CFunction IndexFunc)
{
    int32 Type = luaL_getmetatable(L, Name);
    if (Type == LUA_TTABLE)
    {
        lua_pop(L, 1);
        return true;
    }

    GReflectionRegistry.RegisterEnum(ANSI_TO_TCHAR(Name));

    lua_pop(L, 1);
    luaL_newmetatable(L, Name);
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, IndexFunc);
    lua_rawset(L, -3);
    SetTableForClass(L, Name);
    return true;
}

/**
 * __index meta methods for collision related enums
 */
static int32 CollisionEnum_Index(lua_State *L, int32(*Converter)(FName))
{
    const char *Name = lua_tostring(L, -1);
    if (Name)
    {
        int32 Value = Converter(FName(Name));
        if (Value == INDEX_NONE)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Cann't find enum %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Name));
        }
        lua_pushvalue(L, 2);
        lua_pushinteger(L, Value);
        lua_rawset(L, 1);
        lua_pushinteger(L, Value);
    }
    else
    {
        lua_pushinteger(L, INDEX_NONE);
    }
    return 1;
}

static int32 ECollisionChannel_Index(lua_State *L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToCollisionChannel);
}

static int32 EObjectTypeQuery_Index(lua_State *L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToObjectType);
}

static int32 ETraceTypeQuery_Index(lua_State *L)
{
    return CollisionEnum_Index(L, &FCollisionHelper::ConvertToTraceType);
}

/**
 * Register ECollisionChannel
 */
bool RegisterECollisionChannel(lua_State *L)
{
    return RegisterCollisionEnum(L, "ECollisionChannel", ECollisionChannel_Index);
}

/**
 * Register EObjectTypeQuery
 */
bool RegisterEObjectTypeQuery(lua_State *L)
{
    return RegisterCollisionEnum(L, "EObjectTypeQuery", EObjectTypeQuery_Index);
}

/**
 * Register ETraceTypeQuery
 */
bool RegisterETraceTypeQuery(lua_State *L)
{
    return RegisterCollisionEnum(L, "ETraceTypeQuery", ETraceTypeQuery_Index);
}

/**
 * Clear metatable references
 */
void ClearLibrary(lua_State *L, const char *LibrayName)
{
    lua_pushnil(L);
    SetTableForClass(L, LibrayName);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LibrayName);
}

/**
 * Create weak key table
 */
void CreateWeakKeyTable(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "k");
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
}

/**
 * Create weak value table
 */
void CreateWeakValueTable(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "v");
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
}

/**
 * Debug only...
 */
bool PeekTableElement(lua_State *L, void*)
{
    int32 KeyType = lua_type(L, -2);
    switch (KeyType)
    {
    case LUA_TBOOLEAN:
        {
            int32 b = lua_toboolean(L, -2);
            check(b >= 0);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        {
            const void *p = lua_topointer(L, -2);
            check(true);
        }
        break;
    case LUA_TNUMBER:
        {
            float f = lua_tonumber(L, -2);
            check(true);
        }
        break;
    case LUA_TSTRING:
        {
            const char *s = lua_tostring(L, -2);
            check(true);
        }
        break;
    case LUA_TUSERDATA:
        {
            const void *p = lua_topointer(L, -2);
            check(true);
        }
        break;
    }
    int32 ValueType = lua_type(L, -1);
    switch (ValueType)
    {
    case LUA_TBOOLEAN:
        {
            int32 b = lua_toboolean(L, -1);
            check(b >= 0);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        {
            const void *p = lua_topointer(L, -1);
            check(true);
        }
        break;
    case LUA_TNUMBER:
        {
            float f = lua_tonumber(L, -1);
            check(true);
        }
        break;
    case LUA_TSTRING:
        {
            const char *s = lua_tostring(L, -1);
            check(true);
        }
        break;
    case LUA_TUSERDATA:
        {
            if (lua_checkstack(L, 2))
            {
                UObject *p = UnLua::GetUObject(L, -1);
                UStruct *Struct = Cast<UStruct>(p);
                if (Struct && Struct->IsNative())
                {
                    return false;
                }
                check(true);
            }
        }
        break;
    }
    return true;
}

/**
 * Traverse a Lua table
 */
int32 TraverseTable(lua_State *L, int32 Index, void *Userdata, bool(*TraverseWorker)(lua_State*, void*))
{
    if (Index < 0 && Index > LUA_REGISTRYINDEX)
    {
        int32 Top = lua_gettop(L);
        Index = Top + Index + 1;
    }
    int32 Type = lua_type(L, Index);
    if (Type == LUA_TTABLE)
    {
        if (!lua_checkstack(L, 2))
        {
            return INDEX_NONE;
        }

        int32 NumElements = 0;
        lua_pushnil(L);
        while (lua_next(L, Index) != 0)
        {
            if (TraverseWorker)
            {
                bool b = TraverseWorker(L, Userdata);
                if (b)
                {
                    ++NumElements;
                }
            }
            lua_pop(L, 1);
        }
        return NumElements;
    }
    return INDEX_NONE;
}

int32 Global_RegisterEnum(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    RegisterEnum(L, lua_tostring(L, 1));
    return 0;
}

/**
 * Register an enum (by FEnumDesc)
 */
static bool RegisterEnumInternal(lua_State *L, FEnumDesc *EnumDesc)
{
    if (EnumDesc)
    {
        TStringConversion<TStringConvert<TCHAR, ANSICHAR>> EnumName(*EnumDesc->GetName());
        int32 Type = luaL_getmetatable(L, EnumName.Get());
        if (Type != LUA_TTABLE)
        {
            EnumDesc->AddRef();

            luaL_newmetatable(L, EnumName.Get());

            lua_pushstring(L, "__index");
            lua_pushcfunction(L, Enum_Index);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, Enum_Delete);
            lua_rawset(L, -3);

            // add other members here

            lua_pushvalue(L, -1);               // set metatable to self
            lua_setmetatable(L, -2);

            SetTableForClass(L, EnumName.Get());

            GLuaCxt->AddLibraryName(*EnumDesc->GetName());
        }
        lua_pop(L, 1);
        return true;
    }
    return false;
}

/**
 * Register an enum (by name)
 */
bool RegisterEnum(lua_State *L, const char *EnumName)
{
    if (!EnumName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid enum name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    FString Name(EnumName);
    FEnumDesc *EnumDesc = GReflectionRegistry.RegisterEnum(*Name);
    bool bSuccess = RegisterEnumInternal(L, EnumDesc);
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register enum %s!"), ANSI_TO_TCHAR(__FUNCTION__), *Name);
    }
    return bSuccess;
}

/**
 * Register an enum (by UEnum)
 */
bool RegisterEnum(lua_State *L, UEnum *Enum)
{
    if (!Enum)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid UEnum!"), ANSI_TO_TCHAR(__FUNCTION__));
        return false;
    }

    FEnumDesc *EnumDesc = GReflectionRegistry.RegisterEnum(Enum);
    bool bSuccess = RegisterEnumInternal(L, EnumDesc);
    if (!bSuccess)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Failed to register UEnum!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
    return bSuccess;
}

int32 Global_RegisterClass(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    RegisterClass(L, lua_tostring(L, 1));
    return 0;
}

extern int32 UObject_Identical(lua_State *L);
extern int32 UObject_Delete(lua_State *L);

/**
 * Register a class
 */

static bool RegisterClassCore(lua_State *L, FClassDesc *InClass, const FClassDesc *InSuperClass, UnLua::IExportedClass **ExportedClasses, int32 NumExportedClasses)
{
    check(InClass);
    const TCHAR *InClassName = *InClass->GetName();
    TStringConversion<TStringConvert<TCHAR, ANSICHAR>> ClassName(InClassName);

    int32 Type = luaL_getmetatable(L, ClassName.Get());
    if (Type == LUA_TTABLE)
    {
        UE_LOG(LogUnLua, Verbose, TEXT("%s: Class %s is already registered!"), ANSI_TO_TCHAR(__FUNCTION__), InClassName);
        lua_pop(L, 1);
        return true;
    }

    InClass->AddRef();

    lua_pop(L, 1);
    luaL_newmetatable(L, ClassName.Get());                  // 1, will be used as meta table later (lua_setmetatable)

    const TCHAR *InSuperClassName = nullptr;
    if (InSuperClass)
    {
        InSuperClassName = *InSuperClass->GetName();
        lua_pushstring(L, "ParentClass");                   // 2
        Type = luaL_getmetatable(L, TCHAR_TO_ANSI(InSuperClassName));
        if (Type != LUA_TTABLE)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid super class %s!"), ANSI_TO_TCHAR(__FUNCTION__), InSuperClassName);
        }
        lua_rawset(L, -3);
    }

    lua_pushstring(L, "__index");                           // 2
    lua_pushcfunction(L, Class_Index);                      // 3
    lua_rawset(L, -3);

    lua_pushstring(L, "__newindex");                        // 2
    lua_pushcfunction(L, Class_NewIndex);                   // 3
    lua_rawset(L, -3);

    UScriptStruct *ScriptStruct = InClass->AsScriptStruct();
    if (ScriptStruct)
    {
        lua_pushlightuserdata(L, InClass);                  // FClassDesc

        lua_pushstring(L, "Copy");                          // Key
        lua_pushvalue(L, -2);                               // FClassDesc
        lua_pushcclosure(L, ScriptStruct_Copy, 1);          // closure
        lua_rawset(L, -4);

        lua_pushstring(L, "__eq");                          // Key
        lua_pushvalue(L, -2);                               // FClassDesc
        lua_pushcclosure(L, ScriptStruct_Compare, 1);       // closure
        lua_rawset(L, -4);

        if (!(ScriptStruct->StructFlags & (STRUCT_IsPlainOldData | STRUCT_NoDestructor)))
        {
            lua_pushstring(L, "__gc");                      // Key
            lua_pushvalue(L, -2);                           // FClassDesc
            lua_pushcclosure(L, ScriptStruct_Delete, 1);    // closure
            lua_rawset(L, -4);
        }

        lua_pushstring(L, "__call");                        // Key
        lua_pushvalue(L, -2);                               // FClassDesc
        lua_pushcclosure(L, ScriptStruct_New, 1);           // closure
        lua_rawset(L, -4);

        lua_pop(L, 1);
    }
    else
    {
        UClass *Class = InClass->AsClass();
        if (Class != UObject::StaticClass() && Class != UClass::StaticClass())
        {
            lua_pushstring(L, "ClassDesc");                 // Key
            lua_pushlightuserdata(L, InClass);              // FClassDesc
            lua_rawset(L, -3);

            lua_pushstring(L, "StaticClass");               // Key
            lua_pushlightuserdata(L, InClass);              // FClassDesc
            lua_pushcclosure(L, Class_StaticClass, 1);      // closure
            lua_rawset(L, -3);

            lua_pushstring(L, "Cast");                      // Key
            lua_pushcfunction(L, Class_Cast);               // C function
            lua_rawset(L, -3);

            lua_pushstring(L, "__eq");                      // Key
            lua_pushcfunction(L, UObject_Identical);        // C function
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");                      // Key
            lua_pushcfunction(L, UObject_Delete);           // C function
            lua_rawset(L, -3);
        }
    }

    lua_pushvalue(L, -1);                                   // set metatable to self
    lua_setmetatable(L, -2);

    if (ExportedClasses)
    {
        for (int32 i = 0; i < NumExportedClasses; ++i)
        {
            ExportedClasses[i]->Register(L);
        }
    }

    SetTableForClass(L, ClassName.Get());

    if (!InClass->IsNative())
    {
        GLuaCxt->AddLibraryName(InClassName);
    }

    UE_LOG(LogUnLua, Verbose, TEXT("Succeed to register class %s, super class is %s!"), InClassName, InSuperClassName ? InSuperClassName : TEXT("NULL"));

    return true;
}

static bool RegisterClassInternal(lua_State *L, const FClassDesc *ClassDesc, TArray<FClassDesc*> &Chain)
{
    if (ClassDesc)
    {
        FScopedSafeClass SafeClasses(Chain);

        const FString &Name = ClassDesc->GetName();

        int32 Type = luaL_getmetatable(L, TCHAR_TO_ANSI(*Name));
        bool bSuccess = Type == LUA_TTABLE;
        lua_pop(L, 1);
        if (bSuccess)
        {
            UE_LOG(LogUnLua, Verbose, TEXT("Class %s is already registered!"), *Name);
            return true;
        }

        TArray<UnLua::IExportedClass*> ExportedClasses;
        UnLua::IExportedClass *ExportedClass = GLuaCxt->FindExportedReflectedClass(Chain.Last()->GetFName());   // find statically exported stuff...
        if (ExportedClass)
        {
            ExportedClasses.Add(ExportedClass);
        }
        RegisterClassCore(L, Chain.Last(), nullptr, ExportedClasses.GetData(), ExportedClasses.Num());

        for (int32 i = Chain.Num() - 2; i > -1; --i)
        {
            ExportedClass = GLuaCxt->FindExportedReflectedClass(Chain[i]->GetFName());                          // find statically exported stuff...
            if (ExportedClass)
            {
                ExportedClasses.Add(ExportedClass);
            }
            RegisterClassCore(L, Chain[i], Chain[i + 1], ExportedClasses.GetData(), ExportedClasses.Num());
        }

        return true;
    }
    return false;
}

FClassDesc* RegisterClass(lua_State *L, const char *ClassName, const char *SuperClassName)
{
    if (!ClassName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid class name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }

    FString Name(ClassName);
    TArray<FClassDesc*> Chain;
    FClassDesc *ClassDesc = nullptr;
    if (SuperClassName)
    {
        ClassDesc = GReflectionRegistry.RegisterClass(*Name);
        Chain.Add(ClassDesc);
        GReflectionRegistry.RegisterClass(ANSI_TO_TCHAR(SuperClassName), &Chain);
    }
    else
    {
        ClassDesc = GReflectionRegistry.RegisterClass(*Name, &Chain);
    }
    if (!RegisterClassInternal(L, ClassDesc, Chain))
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register class %s!"), ANSI_TO_TCHAR(__FUNCTION__), *Name);
    }
    return ClassDesc;
}

FClassDesc* RegisterClass(lua_State *L, UStruct *Struct, UStruct *SuperStruct)
{
    if (!Struct)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid UStruct!"), ANSI_TO_TCHAR(__FUNCTION__));
        return nullptr;
    }

    TArray<FClassDesc*> Chain;
    FClassDesc *ClassDesc = nullptr;
    if (SuperStruct)
    {
        ClassDesc = GReflectionRegistry.RegisterClass(Struct);
        Chain.Add(ClassDesc);
        GReflectionRegistry.RegisterClass(SuperStruct, &Chain);
    }
    else
    {
        ClassDesc = GReflectionRegistry.RegisterClass(Struct, &Chain);
    }
    if (!RegisterClassInternal(L, ClassDesc, Chain))
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register UStruct!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
    return ClassDesc;
}

int32 Global_GetUProperty(lua_State *L)
{
    if (lua_islightuserdata(L, 2))
    {
        UnLua::ITypeOps *Property = (UnLua::ITypeOps*)lua_touserdata(L, 2);
        UObject *Object = UnLua::GetUObject(L, 1);
        if (Property && Object)
        {
            Property->Read(L, Object, false);           // get FProperty value
        }
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

int32 Global_SetUProperty(lua_State *L)
{
    if (lua_islightuserdata(L, 2))
    {
        UnLua::ITypeOps *Property = (UnLua::ITypeOps*)lua_touserdata(L, 2);
        UObject *Object = UnLua::GetUObject(L, 1);
        if (Property && Object)
        {
            Property->Write(L, Object, 3);              // set FProperty value
        }
    }
    return 0;
}

extern int32 UObject_Load(lua_State *L);
extern int32 UClass_Load(lua_State *L);

/**
 * Global glue function to load a UObject
 */
int32 Global_LoadObject(lua_State *L)
{
    return UObject_Load(L);
}

/**
 * Global glue function to load a UClass
 */
int32 Global_LoadClass(lua_State *L)
{
    return UClass_Load(L);
}

/**
 * Global glue function to create a UObject
 */
int32 Global_NewObject(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 1));
    if (!Class)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid class!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject *Outer = NumParams > 1 ? UnLua::GetUObject(L, 2) : (UObject*)GetTransientPackage();
    if (!Outer)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid outer!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FName Name = NumParams > 2 ? FName(lua_tostring(L, 3)) : NAME_None;
    //EObjectFlags Flags = NumParams > 3 ? EObjectFlags(lua_tointeger(L, 4)) : RF_NoFlags;

    {
        const char *ModuleName = NumParams > 3 ? lua_tostring(L, 4) : nullptr;
        int32 TableRef = INDEX_NONE;
        if (NumParams > 4 && lua_type(L, 5) == LUA_TTABLE)
        {
            lua_pushvalue(L, 5);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, ANSI_TO_TCHAR(ModuleName), TableRef);
#if ENGINE_MINOR_VERSION < 26
        UObject *Object = StaticConstructObject_Internal(Class, Outer, Name);
#else
        FStaticConstructObjectParameters ObjParams(Class);
        ObjParams.Outer = Outer;
        ObjParams.Name = Name;
        UObject *Object = StaticConstructObject_Internal(ObjParams);
#endif
        if (Object)
        {
            UnLua::PushUObject(L, Object);
        }
        else
        {
            UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to new object for class %s!"), ANSI_TO_TCHAR(__FUNCTION__), *Class->GetName());
            return 0;
        }
    }

    return 1;
}

int32 Global_Print(lua_State *L)
{
    FString StrLog;
    int32 nargs = lua_gettop(L);
    for (int32 i = 1; i <= nargs; ++i)
    {
        const char* arg = luaL_tolstring(L, i, nullptr);
        if (!arg)
        {
            arg = "";
        }
        StrLog += UTF8_TO_TCHAR(arg);
        StrLog += TEXT("    ");
    }

    UE_LOG(LogUnLua, Log, TEXT("== UNLUA_PRINT : %s"), *StrLog);
    if (IsInGameThread())
    {
        UKismetSystemLibrary::PrintString(GWorld, StrLog, false, false);
    }
    return 0;
}


void FindLuaLoader(lua_State *L, const char *name) {
    int i;
    luaL_Buffer msg;  /* to build error message */
    luaL_buffinit(L, &msg);

    lua_getglobal(L, "package");
    /* push 'package.searchers' to index 3 in the stack */
    if (lua_getfield(L, -1, "searchers") != LUA_TTABLE)
        luaL_error(L, "'package.searchers' must be a table");
    /*  iterate over available searchers to find a loader */
    for (i = 1; ; i++) {
        if (lua_rawgeti(L, 4, i) == LUA_TNIL) {  /* no more searchers? */
            lua_pop(L, 1);  /* remove nil */
            luaL_pushresult(&msg);  /* create error message */
            luaL_error(L, "module '%s' not found:%s", name, lua_tostring(L, -1));
        }
        lua_pushstring(L, name);
        lua_call(L, 1, 2);  /* call it */
        if (lua_isfunction(L, -2))  /* did it find a loader? */
            return;  /* module loader found */
        else if (lua_isstring(L, -2)) {  /* searcher returned error message? */
            lua_pop(L, 1);  /* remove extra return */
            luaL_addvalue(&msg);  /* concatenate error message */
        }
        else
            lua_pop(L, 2);  /* remove both returns */
    }
    lua_pop(L, 1);
}

/**
 * Global glue function to require a Lua module
 */
int32 Global_Require(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char *ModuleName = lua_tostring(L, 1);
    if (!ModuleName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    lua_settop(L, 1);       /* LOADED table will be at index 2 */

    lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_getfield(L, 2, ModuleName);
    if (lua_toboolean(L, -1))
    {
        return 1;
    }

    lua_pop(L, 1);

    FString FileName(ANSI_TO_TCHAR(ModuleName));
    FileName.ReplaceInline(TEXT("."), TEXT("/"));
    FString RelativeFilePath = FString::Printf(TEXT("%s.lua"), *FileName);
    bool bSuccess = UnLua::LoadFile(L, RelativeFilePath);
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: use ori require to load file %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(ModuleName));

        const char * name = ModuleName;
        FindLuaLoader(L, name);
        lua_pushstring(L, name);  /* pass name as argument to module loader */
        lua_insert(L, -2);  /* name is 1st argument (before search data) */
        lua_call(L, 2, 1);  /* run loader to load module */
        if (!lua_isnil(L, -1))  /* non-nil return? */
            lua_setfield(L, 2, name);  /* LOADED[name] = returned value */
        if (lua_getfield(L, 2, name) == LUA_TNIL) {   /* module set no value? */
            lua_pushboolean(L, 1);  /* use true as result */
            lua_pushvalue(L, -1);  /* extra copy to be returned */
            lua_setfield(L, 2, name);  /* LOADED[name] = true */
        }
        return 1;
    }

    FString FullFilePath = GLuaSrcFullPath + RelativeFilePath;
    lua_pushvalue(L, 1);
    lua_pushstring(L, TCHAR_TO_UTF8(*FullFilePath));
    lua_call(L, 2, 1);

    if (!lua_isnil(L, -1))
    {
        lua_setfield(L, 2, ModuleName);
    }
    if (lua_getfield(L, 2, ModuleName) == LUA_TNIL)
    {
        lua_pushboolean(L, 1);
        lua_pushvalue(L, -1);
        lua_setfield(L, 2, ModuleName);
    }
    return 1;
}

/**
 * __index meta methods for enum
 */
int32 Enum_Index(lua_State *L)
{
    // 1: meta table of the Enum; 2: entry name in Enum
    
    check(lua_isstring(L, -1));
    lua_pushstring(L, "__name");        // 3
    lua_rawget(L, 1);                   // 3
    check(lua_isstring(L, -1));
    
    const FEnumDesc *Enum = GReflectionRegistry.FindEnum(lua_tostring(L, -1));
    check(Enum);
    int64 Value = Enum->GetValue(lua_tostring(L, 2));
    
    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    lua_pushinteger(L, Value);
    lua_rawset(L, 1);
    lua_pushinteger(L, Value);
    
    return 1;
}

int32 Enum_Delete(lua_State *L)
{
    lua_pushstring(L, "__name");
    int32 Type = lua_rawget(L, 1);
    if (Type == LUA_TSTRING)
    {
        const char *EnumName = lua_tostring(L, -1);
        bool bSuccess = GReflectionRegistry.UnRegisterEnumByName(EnumName);
        check(true);
    }
    lua_pop(L, 1);
    return 0;
}

/**
 * __index meta methods for class
 */
int32 Class_Index(lua_State *L)
{
    GetField(L);
    if (lua_islightuserdata(L, -1))
    {
        UnLua::ITypeOps *Property = (UnLua::ITypeOps*)lua_touserdata(L, -1);
        void *ContainerPtr = GetCppInstance(L, 1);
        if (Property && ContainerPtr)
        {
            Property->Read(L, ContainerPtr, false);
            lua_remove(L, -2);
        }
    }
    return 1;
}

/**
 * __newindex meta methods for class
 */
int32 Class_NewIndex(lua_State *L)
{
    GetField(L);
    if (lua_islightuserdata(L, -1))
    {
        UnLua::ITypeOps *Property = (UnLua::ITypeOps*)lua_touserdata(L, -1);
        void *ContainerPtr = GetCppInstance(L, 1);
        if (Property && ContainerPtr)
        {
            Property->Write(L, ContainerPtr, 3);
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

            UE_LOG(LogUnLua, Warning, TEXT("%s: You are modifying metatable! Please make sure you know what you are doing!"), ANSI_TO_TCHAR(__FUNCTION__));
        }
    }
    lua_pop(L, 1);
    return 0;
}

/**
 * Generic closure to call a UFunction
 */
int32 Class_CallUFunction(lua_State *L)
{
    FFunctionDesc *Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!Function)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid function descriptor!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }
    int32 NumParams = lua_gettop(L);
    int32 NumResults = Function->CallUE(L, NumParams);
    return NumResults;
}

/**
 * Generic closure to call a latent function
 */
int32 Class_CallLatentFunction(lua_State *L)
{
    FFunctionDesc *Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!Function)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid function descriptor!"), ANSI_TO_TCHAR(__FUNCTION__));
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
 * Generic closure to get UClass for a type
 */
int32 Class_StaticClass(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ClassDesc)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid FClassDesc!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UClass *Class = ClassDesc->AsClass();
    if (!Class)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid UClass!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

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

    UObject *Object = UnLua::GetUObject(L, 1);
    if (!Object)
    {
        return 0;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 2));
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

/**
 * Generic closure to create a UScriptStruct instance
 */
int32 ScriptStruct_New(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ClassDesc)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid FClassDesc!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid script struct!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), ClassDesc->GetAnsiName(), ClassDesc->GetUserdataPadding());
    ScriptStruct->InitializeStruct(Userdata);
    return 1;
}

/**
 * Generic GC function for UScriptStruct
 */
int32 ScriptStruct_Delete(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ClassDesc)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Invalid FClassDesc!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid script struct! Perhaps it has been GCed by engine!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    bool bTwoLvlPtr = false;
    void *Userdata = GetUserdataFast(L, 1, &bTwoLvlPtr);
    if (Userdata)
    {
        if (!bTwoLvlPtr)
        {
            ScriptStruct->DestroyStruct(Userdata);
        }
    }
    else
    {
        if (!ScriptStruct->IsNative())
        {
            GObjectReferencer.RemoveObjectRef(ScriptStruct);
        }

        bool bSuccess = GReflectionRegistry.UnRegisterClass(ClassDesc);
        check(true);
    }
    return 0;
}

/**
 * Generic closure to copy a UScriptStruct
 */
int32 ScriptStruct_Copy(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ClassDesc)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid FClassDesc!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid script struct!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *Src = GetCppInstanceFast(L, 1);
    void *Userdata = nullptr;
    if (lua_gettop(L) > 1)
    {
        Userdata = GetCppInstanceFast(L, 2);
        lua_pushvalue(L, 2);
    }
    else
    {
        Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), ClassDesc->GetAnsiName(), ClassDesc->GetUserdataPadding());
        ScriptStruct->InitializeStruct(Userdata);
    }
    ScriptStruct->CopyScriptStruct(Userdata, Src);
    return 1;
}

/**
 * Generic closure to compare two UScriptStructs
 */
int32 ScriptStruct_Compare(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!ClassDesc)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid FClassDesc!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid script struct!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    void *A = GetCppInstanceFast(L, 1);
    void *B = GetCppInstanceFast(L, 2);
    bool bResult = A && B ? ScriptStruct->CompareScriptStruct(A, B, /*PPF_None*/0) : false;
    lua_pushboolean(L, bResult);
    return 1;
}

/**
 * Create a type interface according to Lua parameter's type
 */
TSharedPtr<UnLua::ITypeInterface> CreateTypeInterface(lua_State *L, int32 Index)
{
    if (Index < 0 && Index > LUA_REGISTRYINDEX)
    {
        int32 Top = lua_gettop(L);
        Index = Top + Index + 1;
    }

    TSharedPtr<UnLua::ITypeInterface> TypeInterface;
    int32 Type = lua_type(L, Index);
    switch (Type)
    {
    case LUA_TBOOLEAN:
        TypeInterface = GPropertyCreator.CreateBoolProperty();
        break;
    case LUA_TNUMBER:
        TypeInterface = lua_isinteger(L, Index) > 0 ? GPropertyCreator.CreateIntProperty() : GPropertyCreator.CreateFloatProperty();
        break;
    case LUA_TSTRING:
        TypeInterface = GPropertyCreator.CreateStringProperty();
        break;
    case LUA_TTABLE:
        {
            lua_pushstring(L, "__name");
            Type = lua_rawget(L, Index);
            if (Type == LUA_TSTRING)
            {
                const char *Name = lua_tostring(L, -1);
                FClassDesc *ClassDesc = GReflectionRegistry.FindClass(Name);
                if (ClassDesc)
                {
                    if (ClassDesc->IsClass())
                    {
                        UClass *Class = ClassDesc->AsClass();
                        TypeInterface = GPropertyCreator.CreateObjectProperty(Class);
                    }
                    else
                    {
                        UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
                        TypeInterface = GPropertyCreator.CreateStructProperty(ScriptStruct);
                    }
                }
                else
                {
                    FEnumDesc *EnumDesc = GReflectionRegistry.FindEnum(Name);
                    if (EnumDesc)
                    {
                        TypeInterface = GPropertyCreator.CreateEnumProperty(EnumDesc->GetEnum());
                    }
                    else
                    {
                        TypeInterface = GLuaCxt->FindTypeInterface(lua_tostring(L, -1));
                    }
                }
            }
            lua_pop(L, 1);
        }
        break;
    case LUA_TUSERDATA:
        {
            UClass *Class = Cast<UClass>(UnLua::GetUObject(L, Index));
            if (Class)
            {
                TypeInterface = GPropertyCreator.CreateClassProperty(Class);
            }
        }
        break;
    }

    return TypeInterface;
}
