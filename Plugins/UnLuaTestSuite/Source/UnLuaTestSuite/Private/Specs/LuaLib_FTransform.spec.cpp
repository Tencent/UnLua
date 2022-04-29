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

BEGIN_DEFINE_SPEC(FUnLuaLibFTransformSpec, "UnLua.API.FTransform", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFTransformSpec)

void FUnLuaLibFTransformSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::GetState();
    });

    Describe(TEXT("构造FTransform"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            return UE.FTransform()\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform();
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Translation"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Translation = UE.FVector(0,0,1)\
            return UE.FTransform(Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE.FQuat(0,0,0,1)\
            return UE.FTransform(Rotation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE.FRotator(0,0,0)\
            return UE.FTransform(Rotation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)/Translation"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE.FQuat(0,0,0,1)\
            local Translation = UE.FVector(0,0,1)\
            return UE.FTransform(Rotation, Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity, FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)/Translation"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE.FRotator(0,0,0)\
            local Translation = UE.FVector(0,0,1)\
            return UE.FTransform(Rotation, Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator, FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)/Translation/Scale"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE.FQuat(0,0,0,1)\
            local Translation = UE.FVector(0,0,1)\
            local Scale = UE.FVector(1,1,1) * 2\
            return UE.FTransform(Rotation, Translation, Scale)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity, FVector::UpVector, FVector::OneVector * 2);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)/Translation/Scale"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE.FRotator(0,0,0)\
            local Translation = UE.FVector(0,0,1)\
            local Scale = UE.FVector(0,0,1) * 2\
            return UE.FTransform(Rotation, Translation, Scale)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator, FVector::UpVector, FVector::OneVector * 2);
            TEST_TRUE(Actual.Equals(Expected));
        });
    });

    Describe(TEXT("BlendWith"), [this]
    {
        It(TEXT("将当前Transform与目标Transform混合"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Transform1 = UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(0,0,1))\
            local Transform2 = UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(0,0,1) * 2)\
            Transform1:BlendWith(Transform2, 0.5)\
            return Transform1\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());

            auto Transform1 = FTransform(FQuat::Identity, FVector::UpVector);
            const auto& Transform2 = FTransform(FQuat::Identity, FVector::UpVector * 2);
            Transform1.BlendWith(Transform2, 0.5);
            const auto& Expected = Transform1;

            TEST_TRUE(Actual.Equals(Expected));
        });
    });

    Describe(TEXT("BlendWith"), [this]
    {
        It(TEXT("将当前Transform与目标两个Transform混合"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            const char* Chunk = "\
            local Transform1 = UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(0,0,1))\
            local Transform2 = UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(0,0,2))\
            local Transform3 = UE.FTransform(UE.FQuat(0,0,0,1), UE.FVector(0,0,3))\
            Transform1:Blend(Transform2, Transform3, 0.5)\
            return Transform1\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());

            auto Transform1 = FTransform(FQuat::Identity, FVector::UpVector);
            const auto& Transform2 = FTransform(FQuat::Identity, FVector(0, 0, 2));
            const auto& Transform3 = FTransform(FQuat::Identity, FVector(0, 0, 3));
            Transform1.Blend(Transform2, Transform3, 0.5);
            const auto& Expected = Transform1;

            TEST_TRUE(Actual.Equals(Expected));
        });
    });

    Describe(TEXT("tostring()"), [this]
    {
        It(TEXT("转为字符串"), EAsyncExecution::TaskGraphMainThread, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE.FTransform())");
            const auto& Actual = lua_tostring(L, -1);
            const auto& Expected = FTransform().ToString();
            TEST_EQUAL(Actual, Expected);
        });
    });

    AfterEach([this]
    {
        UnLua::Shutdown();
    });
}

#endif //WITH_DEV_AUTOMATION_TESTS
