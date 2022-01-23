#pragma once
#include "lua.hpp"
#include "ReflectionUtils/EnumDesc.h"

namespace UnLua
{
    namespace CoreLib
    {
        void RegisterEnum(lua_State* L, FEnumDesc* EnumDesc);
    }
}
