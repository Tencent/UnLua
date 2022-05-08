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
#if ENABLE_TYPE_CHECK

struct FUnLuaTest_Issue344 : FUnLuaTestBase
{
    virtual bool InstantTest() override
    {
        return true;
    }

    virtual bool SetUp() override
    {
        FUnLuaTestBase::SetUp();

        GetTestRunner().AddExpectedError(TEXT("invalid owner"), EAutomationExpectedErrorFlags::Contains);

        const auto Chunk = "\
        local obj = UE.UClass.Load('/UnLuaTestSuite/Tests/Misc/BP_UnLuaTestStubActor.BP_UnLuaTestStubActor_C') \
        obj.Int32Property = 1 \
        ";
        UnLua::RunChunk(L, Chunk);

        return true;
    }
};

IMPLEMENT_UNLUA_INSTANT_TEST(FUnLuaTest_Issue344, TEXT("UnLua.Regression.Issue344 在Lua中对UClass_Load出来的对象写入属性后，GC的时候会Crash"))

#endif
#endif //WITH_DEV_AUTOMATION_TESTS
