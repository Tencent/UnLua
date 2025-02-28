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

// 声明外部 C 函数，用于在 Lua 环境中打开 Protobuf 库
extern "C" int luaopen_pb(lua_State* L);
extern "C" int luaopen_pb_unsafe(lua_State* L);

// 模块启动函数
void FLuaProtobufModule::StartupModule()
{
    // 注册静态方法 OnLuaEnvCreated，当 Lua 环境创建时调用
    UnLua::FLuaEnv::OnCreated.AddStatic(&FLuaProtobufModule::OnLuaEnvCreated);
}

// 模块关闭函数
void FLuaProtobufModule::ShutdownModule()
{
    // 移除所有与当前模块相关的 Lua 环境创建事件
    UnLua::FLuaEnv::OnCreated.RemoveAll(this);
}

// Lua 环境创建回调函数
void FLuaProtobufModule::OnLuaEnvCreated(UnLua::FLuaEnv& Env)
{
    // 向 Lua 环境中添加内置加载器 "pb" 和 "pb.unsafe"
    Env.AddBuiltInLoader(TEXT("pb"), luaopen_pb);
    Env.AddBuiltInLoader(TEXT("pb.unsafe"), luaopen_pb_unsafe);
    // 设置 Lua 脚本的搜索路径
    Env.DoString("UnLua.PackagePath = UnLua.PackagePath .. ';/Plugins/UnLuaExtensions/LuaProtobuf/Content/Script/?.lua'");
}

// 模块实现宏，使得引擎能够识别和加载该模块
IMPLEMENT_MODULE(FLuaProtobufModule, LuaProtobuf)