#pragma once
#include "lua.hpp"
#include "ReflectionUtils/ClassDesc.h"

namespace UnLua
{
    namespace CoreLib
    {
        void RegisterStruct(lua_State* L, FClassDesc* InStruct);
    }
}
