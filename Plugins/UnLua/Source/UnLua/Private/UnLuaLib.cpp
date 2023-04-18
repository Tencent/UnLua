#include "UnLuaLib.h"
#include "LowLevel.h"
#include "LuaEnv.h"
#include "UnLuaBase.h"

namespace UnLua
{
    namespace UnLuaLib
    {
        static const char* PACKAGE_PATH_KEY = "PackagePath";

        static FString GetMessage(lua_State* L)
        {
            const auto ArgCount = lua_gettop(L);
            FString Message;
            if (!lua_checkstack(L, ArgCount))
            {
                luaL_error(L, "too many arguments, stack overflow");
                return Message;
            }
            for (int ArgIndex = 1; ArgIndex <= ArgCount; ArgIndex++)
            {
                if (ArgIndex > 1)
                    Message += "\t";
                Message += UTF8_TO_TCHAR(luaL_tolstring(L, ArgIndex, NULL));
            }
            return Message;
        }

        static int LogInfo(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Log, TEXT("%s"), *Msg);
            return 0;
        }

        static int LogWarn(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Warning, TEXT("%s"), *Msg);
            return 0;
        }

        static int LogError(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Error, TEXT("%s"), *Msg);
            return 0;
        }

        static int HotReload(lua_State* L)
        {
#if UNLUA_WITH_HOT_RELOAD
            if (luaL_dostring(L, "require('UnLua.HotReload').reload()") != 0)
            {
                LogError(L);
            }
#endif
            return 0;
        }

        static int Ref(lua_State* L)
        {
            const auto Object = GetUObject(L, -1);
            if (!Object)
                return luaL_error(L, "invalid UObject");

            const auto& Env = FLuaEnv::FindEnvChecked(L);
            Env.GetObjectRegistry()->AddManualRef(L, Object);
            return 1;
        }

        static int Unref(lua_State* L)
        {
            const auto Object = GetUObject(L, -1);
            if (!Object)
                return luaL_error(L, "invalid UObject");

            const auto& Env = FLuaEnv::FindEnvChecked(L);
            Env.GetObjectRegistry()->RemoveManualRef(Object);
            return 0;
        }

        static constexpr luaL_Reg UnLua_Functions[] = {
            {"Log", LogInfo},
            {"LogWarn", LogWarn},
            {"LogError", LogError},
            {"HotReload", HotReload},
            {"Ref", Ref},
            {"Unref", Unref},
            {"FTextEnabled", nullptr},
            {NULL, NULL}
        };

#pragma region Legacy Support

        int32 GetUProperty(lua_State* L)
        {
            auto Ptr = lua_touserdata(L, 2);
            if (!Ptr)
                return 0;

            auto Property = static_cast<TSharedPtr<UnLua::ITypeOps>*>(Ptr);
            if (!Property->IsValid())
                return 0;

            auto Self = GetCppInstance(L, 1);
            if (!Self)
                return 0;

            if (UnLua::LowLevel::IsReleasedPtr(Self))
                return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("attempt to read property '%s' on released object"), *(*Property)->GetName())));

            if (!LowLevel::CheckPropertyOwner(L, (*Property).Get(), Self))
                return 0;

            (*Property)->ReadValue_InContainer(L, Self, false);
            return 1;
        }

        int32 SetUProperty(lua_State* L)
        {
            auto Ptr = lua_touserdata(L, 2);
            if (!Ptr)
                return 0;

            auto Property = static_cast<TSharedPtr<UnLua::ITypeOps>*>(Ptr);
            if (!Property->IsValid())
                return 0;

            auto Self = GetCppInstance(L, 1);
            if (LowLevel::IsReleasedPtr(Self))
                return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("attempt to write property '%s' on released object"), *(*Property)->GetName())));

            if (!LowLevel::CheckPropertyOwner(L, (*Property).Get(), Self))
                return 0;

            (*Property)->WriteValue_InContainer(L, Self, 3);
            return 0;
        }

        static constexpr luaL_Reg UnLua_LegacyFunctions[] = {
            {"GetUProperty", GetUProperty},
            {"SetUProperty", SetUProperty},
            {NULL, NULL}
        };

        static void LegacySupport(lua_State* L)
        {
            static const char* Chunk = R"(
            local rawget = _G.rawget
            local rawset = _G.rawset
            local rawequal = _G.rawequal
            local type = _G.type
            local getmetatable = _G.getmetatable
            local require = _G.require

            local GetUProperty = GetUProperty
            local SetUProperty = SetUProperty

            local NotExist = {}

            local function Index(t, k)
                local mt = getmetatable(t)
                local super = mt
                while super do
                    local v = rawget(super, k)
                    if v ~= nil and not rawequal(v, NotExist) then
                        rawset(t, k, v)
                        return v
                    end
                    super = rawget(super, "Super")
                end

                local p = mt[k]
                if p ~= nil then
                    if type(p) == "userdata" then
                        return GetUProperty(t, p)
                    elseif type(p) == "function" then
                        rawset(t, k, p)
                    elseif rawequal(p, NotExist) then
                        return nil
                    end
                else
                    rawset(mt, k, NotExist)
                end

                return p
            end

            local function NewIndex(t, k, v)
                local mt = getmetatable(t)
                local p = mt[k]
                if type(p) == "userdata" then
                    return SetUProperty(t, p, v)
                end
                rawset(t, k, v)
            end

            local function Class(super_name)
                local super_class = nil
                if super_name ~= nil then
                    super_class = require(super_name)
                end

                local new_class = {}
                new_class.__index = Index
                new_class.__newindex = NewIndex
                new_class.Super = super_class

              return new_class
            end

            _G.Class = Class
            _G.GetUProperty = GetUProperty
            _G.SetUProperty = SetUProperty
            )";

            lua_register(L, "UEPrint", LogInfo);
            luaL_loadstring(L, Chunk);
            lua_newtable(L);
            lua_getglobal(L, LUA_GNAME);
            lua_setfield(L, -2, LUA_GNAME);
            luaL_setfuncs(L, UnLua_LegacyFunctions, 0);
            lua_setupvalue(L, -2, 1);
            lua_pcall(L, 0, LUA_MULTRET, 0);
            lua_getglobal(L, "Class");
            lua_setfield(L, -2, "Class");
        }

#pragma endregion

        static int LuaOpen(lua_State* L)
        {
            lua_newtable(L);
            luaL_setfuncs(L, UnLua_Functions, 0);
            lua_pushstring(L, "Content/Script/?.lua;Plugins/UnLua/Content/Script/?.lua");
            lua_setfield(L, -2, PACKAGE_PATH_KEY);
            return 1;
        }

        int Open(lua_State* L)
        {
            lua_register(L, "print", LogInfo);
            luaL_requiref(L, "UnLua", LuaOpen, 1);
            luaL_dostring(L, R"(
                setmetatable(UnLua, {
                    __index = function(t, k)
                        local ok, result = pcall(require, "UnLua." .. tostring(k))
                        if ok then
                            rawset(t, k, result)
                            return result
                        else
                            t.LogWarn(string.format("failed to load module UnLua.%s\n%s", k, result))
                        end
                    end
                })
            )");

#if UNLUA_ENABLE_FTEXT
            luaL_dostring(L, "UnLua.FTextEnabled = true");
#else
            luaL_dostring(L, "UnLua.FTextEnabled = false");
#endif

#if UNLUA_WITH_HOT_RELOAD
            luaL_dostring(L, R"(
                pcall(function() _G.require = require('UnLua.HotReload').require end)
            )");
#endif

            LegacySupport(L);
            lua_pop(L, 1);
            return 1;
        }

        FString GetPackagePath(lua_State* L)
        {
            lua_getglobal(L, "UnLua");
            checkf(lua_istable(L, -1), TEXT("UnLuaLib not registered"));
            lua_getfield(L, -1, PACKAGE_PATH_KEY);
            const auto PackagePath = lua_tostring(L, -1);
            checkf(PackagePath, TEXT("invalid PackagePath"));
            lua_pop(L, 2);
            return UTF8_TO_TCHAR(PackagePath);
        }

        void SetPackagePath(lua_State* L, const FString& PackagePath)
        {
            lua_getglobal(L, "UnLua");
            checkf(lua_istable(L, -1), TEXT("UnLuaLib not registered"));
            lua_pushstring(L, TCHAR_TO_UTF8(*PackagePath));
            lua_setfield(L, -2, PACKAGE_PATH_KEY);
            lua_pop(L, 2);
        }
    }
}
