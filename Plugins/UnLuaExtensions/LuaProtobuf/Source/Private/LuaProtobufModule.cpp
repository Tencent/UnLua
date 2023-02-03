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

#include "LuaProtobufModule.h"
#include "LuaEnv.h"
#include "pb.h"

extern "C" int luaopen_pb(lua_State *L);
extern "C" int luaopen_pb_unsafe(lua_State *L);

void FLuaProtobufModule::StartupModule()
{
    UnLua::FLuaEnv::OnCreated.AddStatic(&FLuaProtobufModule::OnLuaEnvCreated);
}

void FLuaProtobufModule::ShutdownModule()
{
    UnLua::FLuaEnv::OnCreated.RemoveAll(this);
}

void FLuaProtobufModule::OnLuaEnvCreated(UnLua::FLuaEnv& Env)
{
    Env.AddBuiltInLoader(TEXT("pb"), luaopen_pb);
    Env.AddBuiltInLoader(TEXT("pb.unsafe"), luaopen_pb_unsafe);
    Env.DoString("UnLua.PackagePath = UnLua.PackagePath .. ';/Plugins/UnLuaExtensions/LuaProtobuf/Content/Script/?.lua'");
}

IMPLEMENT_MODULE(FLuaProtobufModule, LuaProtobuf)
