#pragma once
#include "lua.hpp"
#include "ReflectionUtils/ClassDesc.h"

namespace UnLua
{
    namespace CoreLib
    {
        void RegisterScriptStruct(lua_State* L, FClassDesc* InClass);
    }
}
