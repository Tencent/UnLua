# Tencent is pleased to support the open source community by making UnLua available.
#
# Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
#
# Licensed under the MIT License (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy of the License at
#
# http://opensource.org/licenses/MIT
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 2.8)

project(Lua)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT LUA_VERSION)
    set(LUA_VERSION "5.4.3")
endif()

set(LUA_SRC_PATH lua-${LUA_VERSION}/src)
aux_source_directory(${LUA_SRC_PATH} LUA_CORE)
list(REMOVE_ITEM LUA_CORE "${LUA_SRC_PATH}/onelua.c")

if(LUA_COMPILE_AS_CPP)
    set_source_files_properties(${LUA_CORE} PROPERTIES LANGUAGE CXX)
endif()

add_library(Lua STATIC
    ${LUA_CORE}
)

if(WIN32 AND NOT CYGWIN)
    target_compile_definitions(Lua PRIVATE LUA_BUILD_AS_DLL)
endif()