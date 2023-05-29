#include "TutorialObject.h"

#include "LuaCore.h"
#include "UnLua.h"
#include "UnLuaEx.h"

FTutorialObject::FTutorialObject()
{
}

static int32 FTutorialObject_New(lua_State* L)
{
	const auto NumParams = lua_gettop(L);
    if (NumParams != 2)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const char* NameChars = lua_tostring(L, 2);
    if (!NameChars)
    {
        UE_LOG(LogUnLua, Log, TEXT("%s: Invalid tutorial name!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    const auto UserData = NewUserdataWithPadding(L, sizeof(FTutorialObject), "FTutorialObject");
	new(UserData) FTutorialObject(UTF8_TO_TCHAR(NameChars));
    return 1;
}

static const luaL_Reg FTutorialObjectLib[] =
{
    { "__call", FTutorialObject_New },
    { nullptr, nullptr }
};

BEGIN_EXPORT_CLASS(FTutorialObject)
ADD_FUNCTION(GetTitle)
ADD_LIB(FTutorialObjectLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FTutorialObject)
