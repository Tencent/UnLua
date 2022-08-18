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

BEGIN_DEFINE_SPEC(FUnLuaLibFLinearColorSpec, "UnLua.API.FLinearColor", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFLinearColorSpec)

void FUnLuaLibFLinearColorSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造FLinearColor"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FLinearColor()");
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(EForceInit::ForceInit));
        });

        It(TEXT("指定RGB"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FLinearColor(0.1,0.2,0.3)");
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f));
        });

        It(TEXT("指定RGBA"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return UE.FLinearColor(0.1,0.2,0.3,0.4)");
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f,0.4f));
        });
    });

    Describe(TEXT("Set"), [this]
    {
        It(TEXT("设置R"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FLinearColor()\
            Color:Set(0.1)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0,0,0));
        });

        It(TEXT("设置RG"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FLinearColor()\
            Color:Set(0.1,0.2)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0,0));
        });

        It(TEXT("设置RGB"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FLinearColor()\
            Color:Set(0.1,0.2,0.3)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f,0));
        });

        It(TEXT("设置RGBA"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color = UE.FLinearColor()\
            Color:Set(0.1,0.2,0.3,0.4)\
            return Color\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f,0.4f));
        });
    });

    Describe(TEXT("Add"), [this]
    {
        It(TEXT("颜色相加"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color1 = UE.FLinearColor(0.1,0.2,0.3)\
            local Color2 = UE.FLinearColor(0.4,0.5,0.6)\
            return Color1 + Color2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f)+FLinearColor(0.4f,0.5f,0.6f));
        });
    });

    Describe(TEXT("Sub"), [this]
    {
        It(TEXT("颜色相减"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color1 = UE.FLinearColor(0.4,0.5,0.6)\
            local Color2 = UE.FLinearColor(0.1,0.2,0.3)\
            return Color1 - Color2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.4f,0.5f,0.6f) - FLinearColor(0.1f,0.2f,0.3f));
        });
    });

    Describe(TEXT("Mul"), [this]
    {
        It(TEXT("颜色相乘"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color1 = UE.FLinearColor(0.1,0.2,0.3)\
            local Color2 = UE.FLinearColor(0.4,0.5,0.6)\
            return Color1 * Color2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f) * FLinearColor(0.4f,0.5f,0.6f));
        });
    });

    Describe(TEXT("Div"), [this]
    {
        It(TEXT("颜色相除"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Color1 = UE.FLinearColor(0.1,0.2,0.3)\
            local Color2 = UE.FLinearColor(0.4,0.5,0.6)\
            return Color1 / Color2\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto Color = UnLua::Get<FLinearColor>(L, -1, UnLua::TType<FLinearColor>());
            TEST_EQUAL(Color, FLinearColor(0.1f,0.2f,0.3f) / FLinearColor(0.4f,0.5f,0.6f));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE.FLinearColor(1,2,3,4))");
            TEST_EQUAL(lua_tostring(L, -1), "(R=1.000000,G=2.000000,B=3.000000,A=4.000000)");
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
