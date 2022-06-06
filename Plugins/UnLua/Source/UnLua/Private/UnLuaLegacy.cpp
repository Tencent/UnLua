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

#include "UnLuaEx.h"
#include "UnLuaPrivate.h"

DEFINE_STAT(STAT_UnLua_Lua_Memory);
DEFINE_STAT(STAT_UnLua_PersistentParamBuffer_Memory);
DEFINE_STAT(STAT_UnLua_OutParmRec_Memory);
DEFINE_STAT(STAT_UnLua_ContainerElementCache_Memory);

namespace UnLua
{
    /**
     * Exported enum
     */
    void FExportedEnum::Register(lua_State *L)
    {
        TStringConversion<TStringConvert<TCHAR, ANSICHAR>> EnumName(*Name);

        int32 Type = luaL_getmetatable(L, EnumName.Get());
        lua_pop(L, 1);
        if (Type == LUA_TTABLE)
        {
            return;
        }

        luaL_newmetatable(L, EnumName.Get());

        for (TMap<FString, int32>::TIterator It(NameValues); It; ++It)
        {
            lua_pushstring(L, TCHAR_TO_UTF8(*It.Key()));
            lua_pushinteger(L, It.Value());
            lua_rawset(L, -3);
        }

        lua_getglobal(L, "UE");
        lua_pushstring(L, EnumName.Get());
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);
        lua_pop(L, 2);
    }

#if WITH_EDITOR
    void FExportedEnum::GenerateIntelliSense(FString &Buffer) const
    {
        Buffer = FString::Printf(TEXT("---@class %s\r\n"), *Name);

        for (TMap<FString, int32>::TConstIterator It(NameValues); It; ++It)
        {
            Buffer += FString::Printf(TEXT("---@field %s %s %s\r\n"), TEXT("public"), *It.Key(), TEXT("integer"));
        }

        Buffer += TEXT("local M = {}\r\n\r\n");
        Buffer += TEXT("return M\r\n");
    }
#endif

} // namespace UnLua

FString GLuaSrcRelativePath = TEXT("Script/");
FString GLuaSrcFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + GLuaSrcRelativePath);
