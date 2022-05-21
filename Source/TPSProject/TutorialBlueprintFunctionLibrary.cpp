#include "TutorialBlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "UnLua.h"

static void PrintScreen(const FString& Msg)
{
    UKismetSystemLibrary::PrintString(nullptr, Msg, true, false, FLinearColor(0, 0.66, 1), 100);
}

void UTutorialBlueprintFunctionLibrary::CallLuaByGlobalTable()
{
    PrintScreen(TEXT("[C++]CallLuaByGlobalTable 开始"));

    UnLua::FLuaEnv Env;
    const auto bSuccess = Env.DoString("G_08_CppCallLua = require 'Tutorials.08_CppCallLua'");
    check(bSuccess);

    const auto RetValues = UnLua::CallTableFunc(Env.GetMainState(), "G_08_CppCallLua", "CallMe", 1.1f, 2.2f);
    check(RetValues.Num() == 1);

    const auto Msg = FString::Printf(TEXT("[C++]收到来自Lua的返回，结果=%f"), RetValues[0].Value<float>());
    PrintScreen(Msg);
    PrintScreen(TEXT("[C++]CallLuaByGlobalTable 结束"));
}

void UTutorialBlueprintFunctionLibrary::CallLuaByFLuaTable()
{
    PrintScreen(TEXT("[C++]CallLuaByFLuaTable 开始"));
    UnLua::FLuaEnv Env;

    const auto Require = UnLua::FLuaFunction(&Env, "_G", "require");
    const auto RetValues1 = Require.Call("Tutorials.08_CppCallLua");
    check(RetValues1.Num() == 2);

    const auto RetValue = RetValues1[0];
    const auto LuaTable = UnLua::FLuaTable(&Env, RetValue);
    const auto RetValues2 = LuaTable.Call("CallMe", 3.3f, 4.4f);
    check(RetValues2.Num() == 1);

    const auto Msg = FString::Printf(TEXT("[C++]收到来自Lua的返回，结果=%f"), RetValues2[0].Value<float>());
    PrintScreen(Msg);
    PrintScreen(TEXT("[C++]CallLuaByFLuaTable 结束"));
}

bool CustomLoader1(UnLua::FLuaEnv& Env, const FString& RelativePath, TArray<uint8>& Data, FString& FullPath)
{
    const auto SlashedRelativePath = RelativePath.Replace(TEXT("."), TEXT("/"));
    FullPath = FString::Printf(TEXT("%s%s.lua"), *GLuaSrcFullPath, *SlashedRelativePath);

    if (FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
        return true;

    FullPath.ReplaceInline(TEXT(".lua"), TEXT("/Index.lua"));
    if (FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
        return true;

    return false;
}

bool CustomLoader2(UnLua::FLuaEnv& Env, const FString& RelativePath, TArray<uint8>& Data, FString& FullPath)
{
    const auto SlashedRelativePath = RelativePath.Replace(TEXT("."), TEXT("/"));
    const auto L = Env.GetMainState();
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char* Path = lua_tostring(L, -1);
    lua_pop(L, 2);
    if (!Path)
        return false;

    TArray<FString> Parts;
    FString(Path).ParseIntoArray(Parts, TEXT(";"), false);
    for (auto& Part : Parts)
    {
        Part.ReplaceInline(TEXT("?"), *SlashedRelativePath);
        FPaths::CollapseRelativeDirectories(Part);
        
        if (FPaths::IsRelative(Part))
            FullPath = FPaths::ConvertRelativePathToFull(GLuaSrcFullPath, Part);
        else
            FullPath = Part;

        if (FFileHelper::LoadFileToArray(Data, *FullPath, FILEREAD_Silent))
            return true;
    }

    return false;
}

void UTutorialBlueprintFunctionLibrary::SetupCustomLoader(int Index)
{
    switch (Index)
    {
    case 0:
        FUnLuaDelegates::CustomLoadLuaFile.Unbind();
        break;
    case 1:
        FUnLuaDelegates::CustomLoadLuaFile.BindStatic(CustomLoader1);
        break;
    case 2:
        FUnLuaDelegates::CustomLoadLuaFile.BindStatic(CustomLoader2);
        break;
    }
}
