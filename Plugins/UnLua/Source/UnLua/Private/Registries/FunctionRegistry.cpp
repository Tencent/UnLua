#include "FunctionRegistry.h"
#include "lua.hpp"
#include "LuaEnv.h"

namespace UnLua
{
    FFunctionRegistry::FFunctionRegistry(FLuaEnv* Env)
        : Env(Env)
    {
    }

    void FFunctionRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        const auto Function = (ULuaFunction*)Object;
        const auto Info = LuaFunctions.Find(Function);
        if (!Info)
            return;
        luaL_unref(Env->GetMainState(), LUA_REGISTRYINDEX, Info->LuaRef);
        LuaFunctions.Remove(Function);
    }

    void FFunctionRegistry::Invoke(ULuaFunction* Function, UObject* Context, FFrame& Stack, RESULT_DECL)
    {
        // TODO: refactor
        if (UNLIKELY(!Env->GetObjectRegistry()->IsBound(Context)))
            Env->TryBind(Context);

        const auto SelfRef = Env->GetObjectRegistry()->GetBoundRef(Context);
        check(SelfRef!=LUA_NOREF);

        const auto L = Env->GetMainState();
        lua_Integer FuncRef;
        FFunctionDesc* FuncDesc;

        const auto Exists = LuaFunctions.Find(Function);
        if (Exists)
        {
            FuncRef = Exists->LuaRef;
            FuncDesc = Exists->Desc.Get();
        }
        else
        {
            FuncRef = LUA_NOREF;
            FuncDesc = new FFunctionDesc(Function, nullptr);

            lua_rawgeti(L, LUA_REGISTRYINDEX, SelfRef);
            lua_getmetatable(L, -1);
            do
            {
                lua_pushstring(L, FuncDesc->GetLuaFunctionName());
                lua_rawget(L, -2);
                if (lua_isfunction(L, -1))
                {
                    lua_pushvalue(L, -3);
                    lua_remove(L, -3);
                    lua_remove(L, -3);
                    lua_pushvalue(L, -2);
                    FuncRef = luaL_ref(L, LUA_REGISTRYINDEX);
                    break;
                }
                lua_pop(L, 1);
                lua_pushstring(L, "Super");
                lua_rawget(L, -2);
                lua_remove(L, -2);
            }
            while (lua_istable(L, -1));
            lua_pop(L, 2);
            
            FFunctionInfo Info;
            Info.LuaRef = FuncRef;
            Info.Desc = TUniquePtr<FFunctionDesc>(FuncDesc);
            LuaFunctions.Add(Function, MoveTemp(Info));
        }

        if (FuncRef == LUA_NOREF)
        {
            // 可能因为Lua模块加载失败导致找不到对应的function，转发给原函数
            const auto Overridden = Function->GetOverridden();
            if (Overridden && Stack.Code)
                Overridden->Invoke(Context, Stack, RESULT_PARAM);
            return;
        }
        FuncDesc->CallLua(L, FuncRef, SelfRef, Stack, RESULT_PARAM);
    }
}
