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

#include "UnLuaBase.h"
#include "UnLuaTemplate.h"
#include "Misc/AutomationTest.h"
#include "UnLuaTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibFColorSpec, "UnLua.API.FColor", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFColorSpec)

void FUnLuaLibFColorSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造FColor"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FColor()");
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(ForceInitToZero));
        });

        It(TEXT("指定ColorValue"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FColor(0x01020304)");
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(0x01020304));
        });

        It(TEXT("指定RGB"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FColor(1,2,3)");
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,2,3));
        });

        It(TEXT("指定RGBA"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FColor(1,2,3,4)");
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,2,3,4));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置R"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FColor()\
            Color:Set(1)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,0,0,0));
        });

        It(TEXT("设置RG"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FColor()\
            Color:Set(1,2)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,2,0,0));
        });

        It(TEXT("设置RGB"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FColor()\
            Color:Set(1,2,3)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,2,3,0));
        });

        It(TEXT("设置RGBA"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FColor()\
            Color:Set(1,2,3,4)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(1,2,3,4));
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("颜色相加"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color1 = UE.FColor(1,2,3)\
            local Color2 = UE.FColor(4,5,6)\
            return Color1 + Color2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FColor>(L, -1, UnLua::TType<FColor>());
            TEST_EQUAL(Color, FColor(5,7,9));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE.FColor(1,2,3,4))");
            TEST_EQUAL(lua_tostring(L, -1), "(R=1,G=2,B=3,A=4)");
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
