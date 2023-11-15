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

#include "Registries/ClassRegistry.h"
#include "UnLuaCompatibility.h"
#include "LuaEnv.h"
#include "Binding.h"
#include "LowLevel.h"
#include "LuaCore.h"
#include "UELib.h"
#include "ReflectionUtils/ClassDesc.h"

extern int32 UObject_Identical(lua_State* L);
extern int32 UObject_Delete(lua_State* L);

namespace UnLua
{
    FClassRegistry::FClassRegistry(FLuaEnv* Env)
        : Env(Env)
    {
    }

    FClassRegistry::~FClassRegistry()
    {
        for (const auto Pair : Name2Classes)
            delete Pair.Value;
    }

    void FClassRegistry::Initialize()
    {
        Register("UObject");
        Register("UClass");
    }

    FClassDesc* FClassRegistry::Find(const char* TypeName)
    {
        const FName Key = TypeName;
        FClassDesc** Ret = Name2Classes.Find(Key);
        return Ret ? *Ret : nullptr;
    }

    FClassDesc* FClassRegistry::Find(const UStruct* Type)
    {
        FClassDesc** Ret = Classes.Find(Type);
        return Ret ? *Ret : nullptr;
    }

    FClassDesc* FClassRegistry::RegisterReflectedType(const char* MetatableName)
    {
        FClassDesc* Ret = Find(MetatableName);
        if (Ret)
        {
            Classes.FindOrAdd(Ret->AsStruct(), Ret);
            return Ret;
        }

        const char* TypeName = MetatableName[0] == 'U' || MetatableName[0] == 'A' || MetatableName[0] == 'F' ? MetatableName + 1 : MetatableName;
        const auto Type = LoadReflectedType(TypeName);
        if (!Type)
            return nullptr;

        const auto StructType = Cast<UStruct>(Type);
        if (StructType)
        {
            Ret = RegisterInternal(StructType, UTF8_TO_TCHAR(MetatableName));
            return Ret;
        }

        const auto EnumType = Cast<UEnum>(Type);
        if (EnumType)
        {
            // TODO:
            check(false);
        }

        return nullptr;
    }

    FClassDesc* FClassRegistry::RegisterReflectedType(UStruct* Type)
    {
        FClassDesc** Exists = Classes.Find(Type);
        if (Exists)
            return *Exists;

        const auto MetatableName = LowLevel::GetMetatableName(Type);
        Exists = Name2Classes.Find(FName(MetatableName));
        if (Exists)
        {
            Classes.Add(Type, *Exists);
            return *Exists;
        }

        const auto Ret = RegisterInternal(Type, MetatableName);
        return Ret;
    }

    bool FClassRegistry::PushMetatable(lua_State* L, const char* MetatableName)
    {
        int Type = luaL_getmetatable(L, MetatableName);
        if (Type == LUA_TTABLE)
        {
            FClassDesc* Ret = Find(MetatableName);
            if (Ret && Ret->IsClass() && !Ret->IsStructValid())
            {
                // unregister invalid metatable
                Unregister(Ret, true);
            }
            else
            {
                return true;
            }
        }
        lua_pop(L, 1);

        if (FindExportedNonReflectedClass(MetatableName))
            return false;

        FClassDesc* ClassDesc = RegisterReflectedType(MetatableName);
        if (!ClassDesc)
            return false;

        luaL_newmetatable(L, MetatableName);
        lua_pushstring(L, "__index");
        lua_pushcfunction(L, Class_Index);
        lua_rawset(L, -3);

        lua_pushstring(L, "__newindex");
        lua_pushcfunction(L, Class_NewIndex);
        lua_rawset(L, -3);

        uint64 TypeHash = (uint64)ClassDesc->AsStruct();
        lua_pushstring(L, "TypeHash");
        lua_pushnumber(L, TypeHash);
        lua_rawset(L, -3);

        lua_pushstring(L, "ClassDesc");
        lua_pushlightuserdata(L, ClassDesc);
        lua_rawset(L, -3);

        UScriptStruct* ScriptStruct = ClassDesc->AsScriptStruct();
        if (ScriptStruct)
        {
            lua_pushstring(L, "__index");
            lua_pushcfunction(L, ScriptStruct_Index);
            lua_rawset(L, -3);

            lua_pushlightuserdata(L, ClassDesc);

            lua_pushstring(L, "Copy");
            lua_pushvalue(L, -2);
            lua_pushcclosure(L, ScriptStruct_Copy, 1);
            lua_rawset(L, -4);

            lua_pushstring(L, "CopyFrom");
            lua_pushvalue(L, -2);
            lua_pushcclosure(L, ScriptStruct_CopyFrom, 1);
            lua_rawset(L, -4);

            lua_pushstring(L, "__eq");
            lua_pushvalue(L, -2);
            lua_pushcclosure(L, ScriptStruct_Compare, 1);
            lua_rawset(L, -4);

            lua_pushstring(L, "__gc");
            lua_pushvalue(L, -2);
            lua_pushcclosure(L, ScriptStruct_Delete, 1);
            lua_rawset(L, -4);

            lua_pushstring(L, "__call");
            lua_pushvalue(L, -2);
            lua_pushcclosure(L, ScriptStruct_New, 1); // closure
            lua_rawset(L, -4);

            lua_pop(L, 1);
        }
        else
        {
            UClass* Class = ClassDesc->AsClass();

            if (Class != UObject::StaticClass() && Class != UClass::StaticClass())
            {
                lua_pushstring(L, "StaticClass");
                lua_pushlightuserdata(L, ClassDesc);
                lua_pushcclosure(L, Class_StaticClass, 1);
                lua_rawset(L, -3);

                lua_pushstring(L, "Cast");
                lua_pushcfunction(L, Class_Cast);
                lua_rawset(L, -3);

                lua_pushstring(L, "__eq");
                lua_pushcfunction(L, UObject_Identical);
                lua_rawset(L, -3);

                lua_pushstring(L, "__gc");
                lua_pushcfunction(L, UObject_Delete);
                lua_rawset(L, -3);
            }
        }

        lua_pushvalue(L, -1); // set metatable to self
        lua_setmetatable(L, -2);

        TArray<FClassDesc*> ClassDescChain;
        ClassDescChain.Add(ClassDesc);

        UStruct* SuperStruct = ClassDesc->AsStruct()->GetInheritanceSuper();
        while (SuperStruct)
        {
            ClassDescChain.Add(RegisterReflectedType(SuperStruct));
            SuperStruct = SuperStruct->GetInheritanceSuper();
        }

        TArray<IExportedClass*> ExportedClasses;
        for (int32 i = ClassDescChain.Num() - 1; i > -1; --i)
        {
            auto ExportedClass = FindExportedReflectedClass(*ClassDescChain[i]->GetName());
            if (ExportedClass)
                ExportedClass->Register(L);
        }

        UELib::SetTableForClass(L, MetatableName);

        return true;
    }

    bool FClassRegistry::TrySetMetatable(lua_State* L, const char* MetatableName)
    {
        if (!PushMetatable(L, MetatableName))
            return false;

        lua_setmetatable(L, -2);
        return true;
    }

    FClassDesc* FClassRegistry::Register(const char* MetatableName)
    {
        const auto L = Env->GetMainState();
        if (!PushMetatable(L, MetatableName))
            return nullptr;

        // TODO: refactor
        lua_pop(L, 1);
        FName Key = FName(UTF8_TO_TCHAR(MetatableName));
        return Name2Classes.FindChecked(Key);
    }

    FClassDesc* FClassRegistry::Register(const UStruct* Class)
    {
        const auto MetatableName = LowLevel::GetMetatableName(Class);
        return Register(TCHAR_TO_UTF8(*MetatableName));
    }

    void FClassRegistry::Unregister(const UStruct* Class)
    {
        const auto Desc = Find(Class);
        if (!Desc)
            return;
        Classes.Remove(Class);
        Desc->UnLoad();
        Unregister(Desc, true);
    }

    void FClassRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        Unregister((UStruct*)Object);
    }

    UField* FClassRegistry::LoadReflectedType(const char* InName)
    {
        FString Name = UTF8_TO_TCHAR(InName);

        // find candidates in memory
        UField* Ret = FindFirstObject<UClass>(*Name);
        if (!Ret)
            Ret = FindFirstObject<UScriptStruct>(*Name);
        if (!Ret)
            Ret = FindFirstObject<UEnum>(*Name);

        // load candidates if not found
        if (!Ret)
            Ret = LoadObject<UClass>(nullptr, *Name);
        if (!Ret)
            Ret = LoadObject<UScriptStruct>(nullptr, *Name);
        if (!Ret)
            Ret = LoadObject<UEnum>(nullptr, *Name);

        return Ret;
    }

    FClassDesc* FClassRegistry::RegisterInternal(UStruct* Type, const FString& Name)
    {
        check(Type);
        check(!Classes.Contains(Type));

        FClassDesc* ClassDesc = new FClassDesc(Env, Type, Name);
        Classes.Add(Type, ClassDesc);
        Name2Classes.Add(FName(*Name), ClassDesc);

        return ClassDesc;
    }

    void FClassRegistry::Unregister(const FClassDesc* ClassDesc, const bool bForce)
    {
        if (ClassDesc->IsStructValid() && !bForce)
            return;
        const auto L = Env->GetMainState();
        const auto MetatableName = ClassDesc->GetName();
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, TCHAR_TO_UTF8(*MetatableName));
    }
}
