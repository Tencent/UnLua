#pragma once
#include "lua.hpp"
#include "ReflectionUtils/ClassDesc.h"

namespace UnLua
{
    namespace CoreLib
    {
        void RegisterClass(lua_State* L, FClassDesc* InClass);
    }
}
