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

#include "LuaSocketModule.h"
#include "LuaEnv.h"
#include "luasocket.h"
#include "mime.h"

void FLuaSocketModule::StartupModule()
{
    UnLua::FLuaEnv::OnCreated.AddStatic(&FLuaSocketModule::OnLuaEnvCreated);
}

void FLuaSocketModule::ShutdownModule()
{
    UnLua::FLuaEnv::OnCreated.RemoveAll(this);
}

void FLuaSocketModule::OnLuaEnvCreated(UnLua::FLuaEnv& Env)
{
    using namespace UnLuaExtensions::LuaSocket;
    Env.AddBuiltInLoader(TEXT("socket"), luaopen_socket_core);
    Env.AddBuiltInLoader(TEXT("socket.core"), luaopen_socket_core);
    Env.AddBuiltInLoader(TEXT("mime.core"), luaopen_mime_core);
    Env.DoString("UnLua.PackagePath = UnLua.PackagePath .. ';/Plugins/UnLuaExtensions/LuaSocket/Content/Script/?.lua'");
}

IMPLEMENT_MODULE(FLuaSocketModule, LuaSocket)
