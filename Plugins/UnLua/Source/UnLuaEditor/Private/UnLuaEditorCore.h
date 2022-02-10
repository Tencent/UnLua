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

/* 描述对象的Lua绑定状态 */

#pragma once

enum ELuaBindingStatus
{
    // 未编译，未知
    Unknown,

    // 未绑定
    NotBound,

    // 已绑定
    Bound,

    // 已绑定，但没有找到对应的模块
    BoundButInvalid
};

/* 获取指定蓝图对象上的Lua绑定状态 */
ELuaBindingStatus GetBindingStatus(const UBlueprint* Blueprint);
