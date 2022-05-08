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

#include "LowLevel.h"

namespace UnLua
{
    namespace LowLevel
    {
        bool IsReleasedPtr(const void* Ptr)
        {
            return Ptr == ReleasedPtr;
        }

        /**
         * Create weak key table
         */
        void CreateWeakKeyTable(lua_State* L)
        {
            lua_newtable(L);
            lua_newtable(L);
            lua_pushstring(L, "__mode");
            lua_pushstring(L, "k");
            lua_rawset(L, -3);
            lua_setmetatable(L, -2);
        }

        /**
         * Create weak value table
         */
        void CreateWeakValueTable(lua_State* L)
        {
            lua_newtable(L);
            lua_newtable(L);
            lua_pushstring(L, "__mode");
            lua_pushstring(L, "v");
            lua_rawset(L, -3);
            lua_setmetatable(L, -2);
        }

        FString GetMetatableName(const UObject* Object)
        {
            if (!Object)
                return "";

            if (UNLIKELY(Object->IsA<UEnum>()))
            {
                return Object->IsNative() ? Object->GetName() : Object->GetPathName();
            }

            const UStruct* Struct = Cast<UStruct>(Object);
            if (Struct)
                return GetMetatableName(Struct);

            Struct = Object->GetClass();
            return GetMetatableName(Struct);
        }

        FString GetMetatableName(const UStruct* Struct)
        {
            if (!Struct)
                return "";

            if (Struct->IsNative())
                return FString::Printf(TEXT("%s%s"), Struct->GetPrefixCPP(), *Struct->GetName());
            return Struct->GetPathName();
        }
    }
}
