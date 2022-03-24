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

#include "UnLuaDelegates.h"
#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

static bool bTestClassDestructorCalled = false;

class FTestClass
{
public:
	~FTestClass()
	{
		bTestClassDestructorCalled = true;
	}
};

static int TestLuaError(lua_State* L)
{
	FTestClass Test;
	luaL_error(L, "foo");
	return 0;
}


struct FUnLuaTest_Issue386 : FUnLuaTestBase
{
	virtual bool InstantTest() override
	{
		return true;
	}

	virtual bool SetUp() override
	{
		bTestClassDestructorCalled = false;

		FUnLuaDelegates::OnLuaStateCreated.AddLambda([](lua_State* InL)
		{
			lua_register(InL, "TestLuaError", TestLuaError);
		});

		FUnLuaTestBase::SetUp();

		GetTestRunner().AddExpectedError("foo");
		UnLua::RunChunk(L, "TestLuaError()");
		RUNNER_TEST_TRUE(bTestClassDestructorCalled);
		return true;
	}
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue386, TEXT("UnLua.Regression.Issue386 C++类的析构会在luaerror后被跳过"))

#endif //WITH_DEV_AUTOMATION_TESTS
