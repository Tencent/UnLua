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
    set(LUA_VERSION "lua-5.4.3")
endif()

set(LUA_IDSIZE 256 CACHE STRING "gives the maximum size for the description of the source of a function in debug information.")
set(LUA_SRC_PATH ${LUA_VERSION}/src)
aux_source_directory(${LUA_SRC_PATH} LUA_CORE)
list(REMOVE_ITEM LUA_CORE "${LUA_SRC_PATH}/onelua.c")

add_definitions(-DLUA_IDSIZE=${LUA_IDSIZE})
if(LUA_COMPILE_AS_CPP)
    set_source_files_properties(${LUA_CORE} PROPERTIES LANGUAGE CXX)
endif()

if(WIN32 AND NOT CYGWIN)
    add_library(Lua SHARED
        ${LUA_CORE}
    )
elseif(APPLE)
    if(IOS)
        add_definitions(-DLUA_NO_OS_EXECUTE)
        add_library(Lua STATIC
            ${LUA_CORE}
        )
    else()
        set(CMAKE_MACOSX_RPATH 1)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/libLua")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
        add_definitions(-DLUA_USE_MACOSX)
        add_library(Lua SHARED
            ${LUA_CORE}
        )
    endif()
elseif(PS5)
    add_definitions(-DLUA_NO_OS_EXECUTE)
    add_definitions(-DLUA_NO_OS_TMPNAME)
    add_definitions(-DLUA_NO_IO_TMPFILE)
    add_library(Lua STATIC
        ${LUA_CORE}
    )
    target_include_directories(Lua SYSTEM
        PRIVATE ${SCE_PROSPERO_SDK_DIR}/target/include
        PRIVATE ${SCE_PROSPERO_SDK_DIR}/target/include_common
        PRIVATE ${LUA_SRC_PATH}/../src-ps5
    )
else()
    add_library(Lua STATIC
        ${LUA_CORE}
    )
    target_compile_options(Lua PRIVATE -fPIC)
endif()

if(WIN32 AND NOT CYGWIN)
    target_compile_definitions(Lua PRIVATE LUA_BUILD_AS_DLL)
endif()