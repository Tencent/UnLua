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
#include "ReflectionUtils/PropertyDesc.h"
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
    //!!!Fix!!!
    // tvalue opt
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
 * Get lua file full path from relative path
 */
FString GetFullPathFromRelativePath(const FString& RelativePath)
{
    FString FullFilePath = GLuaSrcFullPath + RelativePath;
    FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FString ProjectPersistentDownloadDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPersistentDownloadDir());
    if (!ProjectPersistentDownloadDir.EndsWith("/"))
    {
        ProjectPersistentDownloadDir.Append("/");
    }

    FString RealFullFilePath = FullFilePath.Replace(*ProjectDir, *ProjectPersistentDownloadDir);        // try to load the file from 'ProjectPersistentDownloadDir' first
    if (IFileManager::Get().FileExists(*RealFullFilePath))
    {
        FullFilePath = RealFullFilePath;
    }
    else
    {
        if (!IFileManager::Get().FileExists(*FullFilePath))
        {
            FullFilePath = "";
        }
    }
    return FullFilePath;
}

/**
 * Create 'UE' namespace (a Lua table)
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
    lua_pushvalue(L, -1);
    lua_setglobal(L, "UE4");    // for legacy support only, will be removed in future release
    lua_setglobal(L, "UE");

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
    lua_getglobal(L, "UE");
    lua_pushstring(L, Name);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 2);
#else
    lua_setglobal(L, Name);
#endif
}

#define USERDATA_MAGIC  0x1688
#define BIT_VARIANT_TAG            (1 << 7)         // variant tag for userdata
#define BIT_TWOLEVEL_PTR        (1 << 5)            // two level pointer flag
#define BIT_SCRIPT_CONTAINER    (1 << 4)            // script container (TArray, TSet, TMap) flag

#pragma  pack(push)
#pragma  pack(1)
struct FUserdataDesc
{
    uint16  magic;
    uint8   tag;
    uint8   padding;
};
#pragma  pack(pop)

/**
 * Get 'TValue' from Lua stack
 */
static TValue* GetTValue(lua_State* L, int32 Index)
{
#if 504 == LUA_VERSION_NUM
    CallInfo* ci = L->ci;
    if (Index > 0)
    {
        StkId o = ci->func + Index;
        check(Index <= L->ci->top - (ci->func + 1));
        if (o >= L->top)
        {
            return &G(L)->nilvalue;
        }
        else
        {
            return s2v(o);
        }
    }
    else if (LUA_REGISTRYINDEX < Index)
    {  /* negative index */
        check(Index != 0 && -Index <= L->top - (ci->func + 1));
        return s2v(L->top + Index);
    }
    else if (Index == LUA_REGISTRYINDEX)
    {
        return &G(L)->l_registry;
    }
    else
    {  /* upvalues */
        Index = LUA_REGISTRYINDEX - Index;
        check(Index <= MAXUPVAL + 1);
        if (ttislcf(s2v(ci->func)))
        {
            /* light C function? */
            return &G(L)->nilvalue;  /* it has no upvalues */
        }
        else
        {
            CClosure* func = clCvalue(s2v(ci->func));
            return (Index <= func->nupvalues) ? &func->upvalue[Index - 1] : &G(L)->nilvalue;
        }
    }
#else
    CallInfo* ci = L->ci;
    if (Index > 0)
    {
        TValue* V = ci->func + Index;
        check(Index <= ci->top - (ci->func + 1));
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
            CClosure* Closure = clCvalue(ci->func);
            return (Index <= Closure->nupvalues) ? &Closure->upvalue[Index - 1] : (TValue*)NULL;
        }
    }
#endif
}

static int32 GetTValueType(TValue* Value)
{
#if 504 == LUA_VERSION_NUM
    return ttype(Value);
#else
    return ttnov(Value);
#endif
}

static Udata* GetUdata(TValue* Value)
{
    return uvalue(Value);
}

static void* GetUdataMem(Udata* U)
{
    return getudatamem(U);
}

static int32 GetUdataMemSize(Udata* U)
{
    return U->len;
}

static uint8 GetUdataHeaderSize()
{
    static uint8 HeaderSize = 0;
    if (0 == HeaderSize)
    {
        lua_State* L = UnLua::GetState();
#if 504 == LUA_VERSION_NUM
        uint8* Userdata = (uint8*)lua_newuserdatauv(L, 0, 0);
#else
        uint8* Userdata = (uint8*)lua_newuserdata(L, 0);
#endif
        TValue* Value = GetTValue(L, -1);
        Udata* U = GetUdata(Value);
        HeaderSize = Userdata - (uint8*)U;

        lua_pop(L, 1);
}

    return HeaderSize;
}

/**
 * Calculate padding size for userdata
 */
uint8 CalcUserdataPadding(int32 Alignment)
{
    uint8 HeaderSize = GetUdataHeaderSize();
    uint8 AlignByte = Align(HeaderSize, Alignment);
    return (uint8)(Align(HeaderSize, Alignment) - HeaderSize);      // sizeof(UUdata) == 40
}

static FUserdataDesc* GetUserdataDesc(Udata* U)
{
    FUserdataDesc* UserdataDesc = NULL;

    uint8 DescSize = sizeof(FUserdataDesc);
    int32 UdataMemSize = GetUdataMemSize(U);
    if (DescSize <= UdataMemSize)
    {
        UserdataDesc = (FUserdataDesc*)((uint8*)GetUdataMem(U) + UdataMemSize - DescSize);
        if (USERDATA_MAGIC != UserdataDesc->magic)
        {
            UserdataDesc = NULL;
        }
    }

    return UserdataDesc;
}

static void* NewUserdataWithDesc(lua_State* L, int Size, uint8 Tag, uint8 Padding)
{
#if 504 == LUA_VERSION_NUM
    uint8* Userdata = (uint8*)lua_newuserdatauv(L, Size + Padding + sizeof(FUserdataDesc), 0);
#else
    uint8* Userdata = (uint8*)lua_newuserdata(L, Size + Padding + sizeof(FUserdataDesc));
#endif
    FUserdataDesc* UserdataDesc = (FUserdataDesc*)(Userdata + Size + Padding);
    UserdataDesc->magic = USERDATA_MAGIC;
    UserdataDesc->tag = Tag;
    UserdataDesc->padding = Padding;

    return Userdata;
}

void* NewUserdataWithTwoLvPtrTag(lua_State* L, int Size, void* Object)
{
    void* Userdata = NewUserdataWithDesc(L, Size, (BIT_VARIANT_TAG | BIT_TWOLEVEL_PTR), 0);
    *(void**)Userdata = Object;
    return Userdata;
}

void* NewUserdataWithContainerTag(lua_State* L, int Size)
{
    return NewUserdataWithDesc(L, Size, (BIT_VARIANT_TAG | BIT_SCRIPT_CONTAINER), 0);
}

void* NewUserdataWithPaddingTag(lua_State* L, int Size, uint8 Padding)
{
    return NewUserdataWithDesc(L, Size, BIT_VARIANT_TAG, Padding);
}

void MarkUserdataTwoLvPtrTag(void* Userdata)
{
    Udata* U = (Udata*)((uint8*)Userdata - GetUdataHeaderSize());
    FUserdataDesc* UserdataDesc = GetUserdataDesc(U);
    if (UserdataDesc)
    {
        UserdataDesc->tag = (BIT_VARIANT_TAG | BIT_TWOLEVEL_PTR);
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
    void* Userdata = nullptr;

    TValue* Value = GetTValue(L, Index);
    int32 Type = GetTValueType(Value);
    if (Type == LUA_TUSERDATA)
    {
        Udata* U = GetUdata(Value);
        uint8* Buffer = (uint8*)GetUdataMem(U);
        FUserdataDesc* UserdataDesc = GetUserdataDesc(U);
        if ((UserdataDesc)
            && (UserdataDesc->tag & BIT_VARIANT_TAG))// if the userdata has a variant tag
        {
            bTwoLvlPtr = (UserdataDesc->tag & BIT_TWOLEVEL_PTR) != 0;        // test if the userdata is a two level pointer
            Userdata = bTwoLvlPtr ? Buffer : Buffer + UserdataDesc->padding;    // add padding to userdata if it's not a two level pointer
        }
        else
        {
            Userdata = Buffer;
        }
    }
    else if (Type == LUA_TLIGHTUSERDATA)
    {
        Userdata = pvalue(Value);                                 // get the light userdata
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
bool TryToSetMetatable(lua_State* L, const char* MetatableName, UObject* Object)
{
    int32 Type = LUA_TNIL;

    // exported non reflected class only need check metatable
    const UnLua::IExportedClass* ExportedClass = GLuaCxt->FindExportedNonReflectedClass(MetatableName);
    if (ExportedClass)
    {
        Type = luaL_getmetatable(L, MetatableName);
        if (LUA_TTABLE != Type)
        {
            lua_pop(L, 1);
        }
        else
        {
            lua_setmetatable(L, -2);                                    // set the metatable directly
        }
        return Type == LUA_TTABLE;
    }
    else
    {   
        // other class,check classdesc
        FClassDesc* ClassDesc = GReflectionRegistry.FindClass(MetatableName);
        if (!ClassDesc)
        {
            UnLua::FAutoStack AutoStack;
            ClassDesc = RegisterClass(L, MetatableName);
        }
        Type = luaL_getmetatable(L, MetatableName);
        if (Type != LUA_TTABLE)
        {
            lua_pop(L, 1);
        }
        else
        {
            lua_setmetatable(L, -2);                                    // set the metatable directly
            ClassDesc->AddRef();
        }
        return Type == LUA_TTABLE;
    }
}


FString GetMetatableName(const UObjectBaseUtility* Object)
{   
    static TMap<FString, FString> Class2Metatable;

	if (!GLuaCxt->IsUObjectValid((UObjectBase*)Object))
	{
		return "";
	}

    const TCHAR* PrefixCPP;
    FString ClassName;
    if (Object->IsA<UEnum>()) 
    {
        PrefixCPP = TEXT("E");
        ClassName = Object->GetName();
    }
    else if (Object->IsA<UStruct>())
    {
        PrefixCPP = ((UStruct*)Object)->GetPrefixCPP();
        ClassName = Object->GetName();
    }
    else 
    {
        PrefixCPP = Object->GetClass()->GetPrefixCPP();
        ClassName = Object->GetClass()->GetName();
    }

    FString MetatableName;
    if (FString* MetatableNamePtr = Class2Metatable.Find(ClassName))
    {
        MetatableName = *MetatableNamePtr;
    }
    else
    {
        MetatableName = Class2Metatable.Add(ClassName, FString::Printf(TEXT("%s%s"), PrefixCPP, *ClassName));
    }

	return MetatableName;
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

    void* Userdata = NewUserdataWithPaddingTag(L, Size, PaddingSize); // userdata size must add padding size
    if (MetatableName)
    {
        bool bSuccess = TryToSetMetatable(L, MetatableName);        // set metatable
        if (!bSuccess)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable, metatable name: !"), ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(MetatableName));
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
 * Create a new userdata for a script container
 */
void* NewScriptContainer(lua_State *L, const FScriptContainerDesc &Desc)
{
    void* Userdata = NewUserdataWithContainerTag(L, Desc.GetSize());
    luaL_setmetatable(L, Desc.GetName());   // set metatable
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

        Userdata = NewUserdataWithContainerTag(L, Desc.GetSize());      // create new userdata
        luaL_setmetatable(L, Desc.GetName());               // set metatable
        lua_pushlightuserdata(L, Key);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);                                  // cache it in 'ScriptContainerMap'
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
    TValue* Value = GetTValue(L, Index);
    if ((Value->tt_ & 0x0F) == LUA_TUSERDATA)
    {
        uint8 Flag = (BIT_VARIANT_TAG | BIT_SCRIPT_CONTAINER);              // variant tags

        Udata* U = GetUdata(Value);
        FUserdataDesc* UserdataDesc = GetUserdataDesc(U);
        if (UserdataDesc)
        {
            return (UserdataDesc->tag & Flag) == Flag ? *((void**)GetUdataMem(U)) : nullptr;
        }
    }
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
    FString MetatableName = GetMetatableName(Object);
    if (MetatableName.IsEmpty())
    {
		lua_pushnil(L);
		return;
    }
    
#if UNLUA_ENABLE_DEBUG != 0
	UE_LOG(LogUnLua, Log, TEXT("%s : %p,%s,%s"), ANSI_TO_TCHAR(__FUNCTION__), Object,*Object->GetName(), *MetatableName);
#endif

    NewUserdataWithTwoLvPtrTag(L, sizeof(void*), Object);  // create a userdata and store the UObject address
    bool bSuccess = TryToSetMetatable(L, TCHAR_TO_UTF8(*MetatableName), (UObject*)Object);
	if (!bSuccess)
	{
		UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid metatable,Name %s, Object %s,%p!"), ANSI_TO_TCHAR(__FUNCTION__), *MetatableName, *Object->GetName(), Object);
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
    NewUserdataWithTwoLvPtrTag(L, sizeof(void*), Value);
}

/**
 * Push a delegate
 */
static void PushDelegateElement(lua_State *L, FDelegateProperty *Property, void *Value)
{
    FScriptDelegate *ScriptDelegate = Property->GetPropertyValuePtr(Value);
    FDelegateHelper::PreBind(ScriptDelegate, Property);

    NewUserdataWithTwoLvPtrTag(L, sizeof(void*), ScriptDelegate);
}

/**
 * Push a multicast delegate
 */
static void PushMCDelegateElement(lua_State *L, FMulticastDelegateProperty *Property, void *Value)
{
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 23
    FMulticastScriptDelegate *ScriptDelegate = Property->GetPropertyValuePtr(Value);
#else
    void *ScriptDelegate = Value;
#endif
    FDelegateHelper::PreAdd(ScriptDelegate, Property);

    NewUserdataWithTwoLvPtrTag(L, sizeof(void*), ScriptDelegate);
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

	int OldTop = lua_gettop(L);

    lua_getfield(L, LUA_REGISTRYINDEX, "ObjectMap");
    lua_pushlightuserdata(L, Object);
    lua_newtable(L);                                            // create a Lua table ('INSTANCE')
    PushObjectCore(L, Object);                                  // push UObject ('RAW_UOBJECT')
    lua_pushstring(L, "Object");
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);                                          // INSTANCET.Object = RAW_UOBJECT

	// in some case may occur module or object metatable can 
	// not be found problem
	int32 TypeModule = GetLoadedModule(L, ModuleName);          // push the required module/table ('REQUIRED_MODULE') to the top of the stack
	int32 TypeMetatable = lua_getmetatable(L, -2);              // get the metatable ('METATABLE_UOBJECT') of 'RAW_UOBJECT' 
	if ((TypeModule != LUA_TTABLE)
		||(0 == TypeMetatable))
	{
		lua_pop(L, lua_gettop(L) - OldTop);		
		return LUA_REFNIL;
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
 * Delete the ref of uobject instance
 */
void DeleteUObjectRefs(lua_State* L, UObjectBaseUtility* Object)
{
    if (GLuaCxt->IsUObjectValid(Object))
    {   
#if UNLUA_ENABLE_DEBUG != 0
        UE_LOG(LogUnLua, Log, TEXT("UObject_Delete : %s,%p!"), *Object->GetName(), Object);
#endif
        // unlua ref
        GObjectReferencer.RemoveObjectRef((UObject*)Object);

        // delegate ref, delegate must be clear before object is gced
        if (GLuaCxt->IsEnable())
        {
            FDelegateHelper::Remove((UObject*)Object);
        }
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
static int32 GetField(lua_State* L)
{
    int32 Type = lua_getmetatable(L, 1);       // get meta table of table/userdata (first parameter passed in)
    check(Type == 1 && lua_istable(L, -1));

    lua_pushvalue(L, 2);                    // push key
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
                    luaL_getmetatable(L, "UClass");
                    lua_pushvalue(L, 2);                // push key
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
    if (L)
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

    GReflectionRegistry.RegisterEnum(Name);

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
    if (L)
    {
        lua_pushnil(L);
        SetTableForClass(L, LibrayName);
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, LibrayName);
    }
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
            luaL_newmetatable(L, EnumName.Get());

            lua_pushstring(L, "__index");
            lua_pushcfunction(L, Enum_Index);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, Enum_Delete);
            lua_rawset(L, -3);

            // add other members here
            lua_pushstring(L, "GetMaxValue");
			lua_pushvalue(L, -2);                               // EnumTable
			lua_pushcclosure(L, Enum_GetMaxValue, 1);          // closure
			lua_rawset(L, -3);

			lua_pushstring(L, "GetNameByValue");
			lua_pushvalue(L, -2);                     
			lua_pushcclosure(L, Enum_GetNameByValue, 1);       
			lua_rawset(L, -3);

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

    FEnumDesc *EnumDesc = GReflectionRegistry.RegisterEnum(EnumName);
    bool bSuccess = RegisterEnumInternal(L, EnumDesc);
    if (!bSuccess)
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register enum %s!"), ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(EnumName));
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

int32 Global_UnRegisterClass(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char* ClassName = lua_tostring(L, -1);
    if (!ClassName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    FClassDesc* ClassDesc = GReflectionRegistry.FindClass(ClassName);
    if (ClassDesc)
    {
        GReflectionRegistry.TryUnRegisterClass(ClassDesc);
    }

    return 0;
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
    if (!GReflectionRegistry.IsDescValid(InClass, DESC_CLASS))
    {
        return false;
    }

    FString StrClassName = InClass->GetName();
    FTCHARToUTF8 ClassName(*StrClassName);

    int32 Type = luaL_getmetatable(L, ClassName.Get());
    if (Type == LUA_TTABLE)
    {
        lua_pop(L, 1);
        return true;
    }
    
    lua_pop(L, 1);
    luaL_newmetatable(L, ClassName.Get());                  // 1, will be used as meta table later (lua_setmetatable)

    if (InSuperClass)
    {
        FTCHARToUTF8 InSuperClassName(*InSuperClass->GetName());
        lua_pushstring(L, "ParentClass");                   // 2
        Type = luaL_getmetatable(L, InSuperClassName.Get());
        if (Type != LUA_TTABLE)
        {
            UNLUA_LOGERROR(L, LogUnLua, Warning, TEXT("%s, Invalid super class %s!"), ANSI_TO_TCHAR(__FUNCTION__), *InSuperClass->GetName());
        }
        lua_rawset(L, -3);
    }


    lua_pushstring(L, "__index");                           // 2
    lua_pushcfunction(L, Class_Index);                      // 3
    lua_rawset(L, -3);

    lua_pushstring(L, "__newindex");                        // 2
    lua_pushcfunction(L, Class_NewIndex);                   // 3
    lua_rawset(L, -3);
    uint64 TypeHash = (uint64)InClass->AsStruct();
    lua_pushstring(L, "TypeHash");
    lua_pushnumber(L, TypeHash);
    lua_rawset(L, -3);

    UScriptStruct *ScriptStruct = InClass->AsScriptStruct();
    if (ScriptStruct)
    {
        lua_pushlightuserdata(L, InClass);                  // FClassDesc

        lua_pushstring(L, "Copy");                          // Key
        lua_pushvalue(L, -2);                               // FClassDesc
        lua_pushcclosure(L, ScriptStruct_Copy, 1);          // closure
        lua_rawset(L, -4);

		lua_pushstring(L, "CopyFrom");                          // Key
		lua_pushvalue(L, -2);                               // FClassDesc
		lua_pushcclosure(L, ScriptStruct_CopyFrom, 1);          // closure
		lua_rawset(L, -4);

        lua_pushstring(L, "__eq");                          // Key
        lua_pushvalue(L, -2);                               // FClassDesc
        lua_pushcclosure(L, ScriptStruct_Compare, 1);       // closure
        lua_rawset(L, -4);

        //if (!(ScriptStruct->StructFlags & (STRUCT_IsPlainOldData | STRUCT_NoDestructor)))
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
        GLuaCxt->AddLibraryName(*StrClassName);
    }

    return true;
}

static bool RegisterClassInternal(lua_State *L, FClassDesc *ClassDesc)
{
    if (GReflectionRegistry.IsDescValid((void*)ClassDesc,DESC_CLASS))
    {
        FScopedSafeClass SafeClasses((FClassDesc*)ClassDesc);

        const FString &Name = ClassDesc->GetName();

        int32 Type = luaL_getmetatable(L, TCHAR_TO_UTF8(*Name));
        bool bSuccess = Type == LUA_TTABLE;
        lua_pop(L, 1);
        if (bSuccess)
        {
            return true;
        }

        TArray<FClassDesc*> ClassDescChain;
        ClassDesc->GetInheritanceChain(ClassDescChain);

        // add self
        ClassDescChain.Insert((FClassDesc*)ClassDesc, 0);

        TArray<UnLua::IExportedClass*> ExportedClasses;
        UnLua::IExportedClass *ExportedClass = GLuaCxt->FindExportedReflectedClass(*ClassDescChain.Last()->GetName());   // find statically exported stuff...
        if (ExportedClass)
        {
            ExportedClasses.Add(ExportedClass);
        }
        RegisterClassCore(L, ClassDescChain.Last(), nullptr, ExportedClasses.GetData(), ExportedClasses.Num());

        for (int32 i = ClassDescChain.Num() - 2; i > -1; --i)
        {
            ExportedClass = GLuaCxt->FindExportedReflectedClass(*ClassDescChain[i]->GetName());                          // find statically exported stuff...
            if (ExportedClass)
            {
                ExportedClasses.Add(ExportedClass);
            }
            RegisterClassCore(L, ClassDescChain[i], ClassDescChain[i + 1], ExportedClasses.GetData(), ExportedClasses.Num());
        }

        return true;
    }
    return false;
}

FClassDesc* RegisterClass(lua_State *L, const char *ClassName, const char *SuperClassName)
{
    if (!ClassName)
    {
        return nullptr;
    }

    FClassDesc *ClassDesc = nullptr;
    if (SuperClassName)
    {
        ClassDesc = GReflectionRegistry.RegisterClass(ClassName);
        GReflectionRegistry.RegisterClass(SuperClassName);
    }
    else
    {
        ClassDesc = GReflectionRegistry.RegisterClass(ClassName);
    }

    if (!RegisterClassInternal(L, ClassDesc))
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register class %s!"), ANSI_TO_TCHAR(__FUNCTION__), UTF8_TO_TCHAR(ClassName));
    }
    return ClassDesc;
}

FClassDesc* RegisterClass(lua_State *L, UStruct *Struct, UStruct *SuperStruct)
{
    if (!Struct)
    {
        return nullptr;
    }

    FClassDesc *ClassDesc = nullptr;
    if (SuperStruct)
    {
        ClassDesc = GReflectionRegistry.RegisterClass(Struct);
        GReflectionRegistry.RegisterClass(SuperStruct);
    }
    else
    {
        ClassDesc = GReflectionRegistry.RegisterClass(Struct);
    }


    if (!RegisterClassInternal(L, ClassDesc))
    {
        UE_LOG(LogUnLua, Warning, TEXT("%s: Failed to register UStruct!"), ANSI_TO_TCHAR(__FUNCTION__));
    }
    return ClassDesc;
}

int32 Global_GetUProperty(lua_State *L)
{
    if (lua_islightuserdata(L, 2))
    {   
        bool bValid = false;
        UnLua::ITypeOps* Property = (UnLua::ITypeOps*)lua_touserdata(L, 2);
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

            UObject* Object = UnLua::GetUObject(L, 1);
            if ((bValid)
                && (GLuaCxt->IsUObjectValid(Object)))
            {
                Property->Read(L, Object, false);           // get UProperty value
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

int32 Global_SetUProperty(lua_State *L)
{
    if (lua_islightuserdata(L, 2))
    {
        bool bValid = false;
        UnLua::ITypeOps* Property = (UnLua::ITypeOps*)lua_touserdata(L, 2);
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

            UObject* Object = UnLua::GetUObject(L, 1);
            if ((bValid)
                && (GLuaCxt->IsUObjectValid(Object)))
            {
                Property->Write(L, Object, 3);              // set UProperty value
            }
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
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 26
        UObject* Object = StaticConstructObject_Internal(Class, Outer, Name);
#else
        FStaticConstructObjectParameters ObjParams(Class);
        ObjParams.Outer = Outer;
        ObjParams.Name = Name;
        UObject* Object = StaticConstructObject_Internal(ObjParams);
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

    UE_LOG(LogUnLua, Log, TEXT("UNLUA_PRINT[%d] : %s"), GFrameNumber,*StrLog);
    if (IsInGameThread())
    {
        UKismetSystemLibrary::PrintString(GWorld, StrLog, false, false);
    }
    return 0;
}

int LoadFromBuiltinLibs(lua_State *L)
{
    TCHAR* Name = UTF8_TO_TCHAR(lua_tostring(L, 1));
    const auto Ctx = FLuaContext::Create();
    const auto BuiltinLoaders = Ctx->GetBuiltinLoaders();
    const auto Loader = BuiltinLoaders.Find(Name);
    if(!Loader)
        return 0;
    lua_pushcfunction(L, *Loader);
    return 1;
}

int LoadFromCustomLoader(lua_State *L)
{
    if(!FUnLuaDelegates::CustomLoadLuaFile.IsBound())
        return 0;

    const FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
    
    TArray<uint8> Data;
    FString FullFilePath;
    if(!FUnLuaDelegates::CustomLoadLuaFile.Execute(FileName, Data, FullFilePath))
        return 0;

    const auto Chunk = (const char*)Data.GetData();
    const auto ChunkName = TCHAR_TO_UTF8(*FileName);
    if(!UnLua::LoadChunk(L, Chunk, Data.Num(), ChunkName))
        return luaL_error(L, "file loading from custom loader error");

    return 1;
}

int LoadFromFileSystem(lua_State *L)
{
    FString FileName(UTF8_TO_TCHAR(lua_tostring(L, 1)));
    FileName.ReplaceInline(TEXT("."), TEXT("/"));
    const auto RelativePath = FString::Printf(TEXT("%s.lua"), *FileName);
    const auto FullPath = GetFullPathFromRelativePath(RelativePath);
    TArray<uint8> Data;
    if(!FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
        return 0;

    const auto SkipLen = 3 < Data.Num() && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0;        // skip UTF-8 BOM mark
    const auto ChunkName = TCHAR_TO_UTF8(*RelativePath);
    const auto Chunk = (const char*)(Data.GetData() + SkipLen);
    const auto ChunkSize = Data.Num() - SkipLen;
    if(!UnLua::LoadChunk(L, Chunk, ChunkSize, ChunkName))
        return luaL_error(L, "file loading from file system error");

    return 1;
}

int32 Global_AddToClassWhiteSet(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char* ClassName = lua_tostring(L, 1);
    if (!ClassName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    GReflectionRegistry.AddToClassWhiteSet(UTF8_TO_TCHAR(ClassName));

    return 0;
}

int32 Global_RemoveFromClassWhiteSet(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char* ClassName = lua_tostring(L, 1);
    if (!ClassName)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    GReflectionRegistry.RemoveFromClassWhiteSet(UTF8_TO_TCHAR(ClassName));

    return 0;
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
	if ((!Enum) 
        || (!Enum->IsValid()))
	{
		lua_pop(L, 1);
		return 0;
	}
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
        const char* EnumName = lua_tostring(L, -1);
        const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
        if (EnumDesc)
        {
            GReflectionRegistry.UnRegisterEnum(EnumDesc);
        }
    }
    lua_pop(L, 1);
    return 0;
}

int32 Enum_GetMaxValue(lua_State* L)
{   
    int32 MaxValue = 0;
    
    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L,-1) == LUA_TTABLE)
    {
		lua_pushstring(L, "__name");
		int32 Type = lua_rawget(L, -2);
		if (Type == LUA_TSTRING)
		{
			const char* EnumName = lua_tostring(L, -1);
			const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
			if (EnumDesc)
			{
				UEnum* Enum = EnumDesc->GetEnum();
				if (Enum)
				{
					MaxValue = Enum->GetMaxEnumValue();
				}
			}
		}
		lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_pushinteger(L, MaxValue);
    return 1;
}

int32 Enum_GetNameByValue(lua_State* L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }

    FText ValueName;

    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L, -1) == LUA_TTABLE)
    {
		// enum value
		int64 Value = lua_tointegerx(L, -2, nullptr);

		lua_pushstring(L, "__name");
		int32 Type = lua_rawget(L, -2);
		if (Type == LUA_TSTRING)
		{
			const char* EnumName = lua_tostring(L, -1);
			const FEnumDesc* EnumDesc = GReflectionRegistry.FindEnum(EnumName);
			if (EnumDesc)
			{
				UEnum* Enum = EnumDesc->GetEnum();
				if (Enum)
				{   
					ValueName = Enum->GetDisplayNameTextByValue(Value);
				}
			}
		}
		lua_pop(L, 1);
    }
    lua_pop(L, 1);

    UnLua::Push(L, ValueName);

    return 1;
}

/**
 * __index meta methods for class
 */
int32 Class_Index(lua_State *L)
{
    GetField(L);
    if (lua_islightuserdata(L, -1))
    {   
        bool bValid = false;
		UnLua::ITypeOps *Property = (UnLua::ITypeOps*)lua_touserdata(L, -1);
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

/**
 * __newindex meta methods for class
 */
int32 Class_NewIndex(lua_State *L)
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
				Property->Write(L, ContainerPtr, 3);
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

/**
 * Generic closure to call a UFunction
 */
int32 Class_CallUFunction(lua_State *L)
{
    //!!!Fix!!!
    //delete desc when is not valid
    FFunctionDesc *Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
    if (!GReflectionRegistry.IsDescValidWithObjectCheck(Function,DESC_FUNCTION))
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
int32 Class_CallLatentFunction(lua_State *L)
{
    FFunctionDesc *Function = (FFunctionDesc*)lua_touserdata(L, lua_upvalueindex(1));
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

FClassDesc* Class_CheckParam(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
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

    UClass *Class = ClassDesc->AsClass();
    if (!Class)
    {
        UE_LOG(LogUnLua, Log, TEXT("Class : ClassDesc type is not class(Name : %s, Address : %p)"), *ClassDesc->GetName(),ClassDesc);
        return NULL;
    }

    return ClassDesc;
}

/**
 * Generic closure to get UClass for a type
 */
int32 Class_StaticClass(lua_State *L)
{
    FClassDesc *ClassDesc = Class_CheckParam(L);
	if (!ClassDesc)
	{
        return 0;
    }

    UClass *Class = ClassDesc->AsClass();
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


FClassDesc* ScriptStruct_CheckParam(lua_State *L)
{
    FClassDesc *ClassDesc = (FClassDesc*)lua_touserdata(L, lua_upvalueindex(1));
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

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    if (!ScriptStruct)
    {
        UE_LOG(LogUnLua, Log, TEXT("ScriptStruct : ClassDesc type is not script struct(Name : %s, Address : %p)"), *ClassDesc->GetName(),ClassDesc);
        return NULL;
    }

    return ClassDesc;
}

/**
 * Generic closure to create a UScriptStruct instance
 */
int32 ScriptStruct_New(lua_State *L)
{
    FClassDesc *ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();
    void *Userdata = NewUserdataWithPadding(L, ClassDesc->GetSize(), TCHAR_TO_UTF8(*ClassDesc->GetName()), ClassDesc->GetUserdataPadding());
    ScriptStruct->InitializeStruct(Userdata);

    return 1;
}

/**
 * Generic GC function for UScriptStruct
 */
int32 ScriptStruct_Delete(lua_State *L)
{   
    FClassDesc *ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();

    bool bTwoLvlPtr = false;
    void * Userdata = GetUserdataFast(L, 1, &bTwoLvlPtr);
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
int32 ScriptStruct_CopyFrom(lua_State *L)
{
    FClassDesc *ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();

	void *Src = GetCppInstanceFast(L, 1);
	void *Userdata = nullptr;
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
	ScriptStruct->CopyScriptStruct(Src,Userdata);
	return 1;
}


/**
 * Generic closure to copy a UScriptStruct
 */
int32 ScriptStruct_Copy(lua_State *L)
{
    FClassDesc *ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();

    void *Src = GetCppInstanceFast(L, 1);
    void *Userdata = nullptr;
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
int32 ScriptStruct_Compare(lua_State *L)
{
    FClassDesc *ClassDesc = ScriptStruct_CheckParam(L);
    if (!ClassDesc)
    {
        return 0;
    }

    UScriptStruct *ScriptStruct = ClassDesc->AsScriptStruct();

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
                const char* Name = lua_tostring(L, -1);
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
