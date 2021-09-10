#include "TutorialBlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "LuaCore.h"
#include "UnLua.h"

static void PrintScreen(const FString& Msg)
{
	UKismetSystemLibrary::PrintString(nullptr, Msg, true, false, FLinearColor(0, 0.66, 1), 100);
}

void UTutorialBlueprintFunctionLibrary::CallLuaByGlobalTable()
{
	PrintScreen(TEXT("[C++]CallLuaByGlobalTable 开始"));
	const auto L = UnLua::GetState();
	const auto bSuccess = UnLua::RunChunk(L, "G_08_CppCallLua = require 'Tutorials.08_CppCallLua'");
	check(bSuccess);

	const auto RetValues = UnLua::CallTableFunc(L, "G_08_CppCallLua", "CallMe", 1.1f, 2.2f);
	check(RetValues.Num() == 1);
	
	const auto Msg = FString::Printf(TEXT("[C++]收到来自Lua的返回，结果=%f"), RetValues[0].Value<float>());
	PrintScreen(Msg);
	PrintScreen(TEXT("[C++]CallLuaByGlobalTable 结束"));
}

void UTutorialBlueprintFunctionLibrary::CallLuaByFLuaTable()
{
	PrintScreen(TEXT("[C++]CallLuaByFLuaTable 开始"));
	const auto L = UnLua::GetState();

	const auto Require = UnLua::FLuaFunction("_G", "require");
	const auto RetValues1 = Require.Call("Tutorials.08_CppCallLua");
	check(RetValues1.Num() == 1);
	
	const auto LuaTable = UnLua::FLuaTable(RetValues1[0]);
	const auto RetValues2 = LuaTable.Call("CallMe", 3.3f, 4.4f);
	check(RetValues2.Num() == 1);

	const auto Msg = FString::Printf(TEXT("[C++]收到来自Lua的返回，结果=%f"), RetValues2[0].Value<float>());
	PrintScreen(Msg);
	PrintScreen(TEXT("[C++]CallLuaByFLuaTable 结束"));
}
