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

#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

struct FUnLuaTest_Issue289 : FUnLuaTestBase
{
	virtual bool InstantTest() override
	{
		return true;
	}

	virtual bool SetUp() override
	{
		FUnLuaTestBase::SetUp();

		const auto World = GetWorld();

		const auto ActorClass = LoadClass<AActor>(nullptr, TEXT("/UnLuaTestSuite/Tests/Regression/Issue289/BP_UnLuaTestActor_Issue289.BP_UnLuaTestActor_Issue289_C"));
		const auto Actor = World->SpawnActor(ActorClass);

		const char* Chunk = "\
			return package.loaded['Tests.Regression.Issue289.TestActor']\
            ";
		UnLua::RunChunk(L, Chunk);
		RUNNER_TEST_EQUAL(luaL_typename(L, -1), "table"); // ensure test actor bound to lua

		World->Tick(LEVELTICK_All, SMALL_NUMBER);
		Actor->Destroy();

		CollectGarbage(RF_NoFlags, true);
		CollectGarbage(RF_NoFlags, true);

		UnLua::RunChunk(L, Chunk);
		RUNNER_TEST_EQUAL(luaL_typename(L, -1), "nil");

		return true;
	}
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue289, TEXT("UnLua.Regression.Issue289 UClass在被UEGC后没有释放相应的绑定"))

#endif //WITH_DEV_AUTOMATION_TESTS
