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
#include "Specs/TestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FUnLuaLibFTransformSpec, "UnLua.API.FTransform", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    lua_State* L;
END_DEFINE_SPEC(FUnLuaLibFTransformSpec)

void FUnLuaLibFTransformSpec::Define()
{
    BeforeEach([this]
    {
        UnLua::Startup();
        L = UnLua::CreateState();
    });

    Describe(TEXT("构造FTransform"), [this]()
    {
        It(TEXT("默认参数"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            return UE4.FTransform()\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform();
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Translation"), EAsyncExecution::ThreadPool, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Translation = UE4.FVector(0,0,1)\
            return UE4.FTransform(Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE4.FQuat(0,0,0,1)\
            return UE4.FTransform(Rotation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)"), EAsyncExecution::ThreadPool, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE4.FRotator(0,0,0)\
            return UE4.FTransform(Rotation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)/Translation"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE4.FQuat(0,0,0,1)\
            local Translation = UE4.FVector(0,0,1)\
            return UE4.FTransform(Rotation, Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity, FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)/Translation"), EAsyncExecution::ThreadPool, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE4.FRotator(0,0,0)\
            local Translation = UE4.FVector(0,0,1)\
            return UE4.FTransform(Rotation, Translation)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator, FVector::UpVector);
            TEST_TRUE(Actual.Equals(Expected));
        });

        It(TEXT("指定Rotation(FQuat)/Translation/Scale"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Rotation = UE4.FQuat(0,0,0,1)\
            local Translation = UE4.FVector(0,0,1)\
            local Scale = UE4.FVector(1,1,1) * 2\
            return UE4.FTransform(Rotation, Translation, Scale)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FQuat::Identity, FVector::UpVector, FVector::OneVector * 2);
            TEST_TRUE(Actual.Equals(Expected));
        });

        xIt(TEXT("指定Rotation(FRotator)/Translation/Scale"), EAsyncExecution::ThreadPool, [this]()
        {
            // TODO: not supported currently
            const char* Chunk = "\
            local Rotation = UE4.FRotator(0,0,0)\
            local Translation = UE4.FVector(0,0,1)\
            local Scale = UE4.FVector(0,0,1) * 2\
            return UE4.FTransform(Rotation, Translation, Scale)\
            ";
            UnLua::RunChunk(L, Chunk);
            const auto& Actual = UnLua::Get<FTransform>(L, -1, UnLua::TType<FTransform>());
            const auto& Expected = FTransform(FRotator::ZeroRotator, FVector::UpVector, FVector::OneVector * 2);
            TEST_TRUE(Actual.Equals(Expected));
        });
    });

    Describe(TEXT("BlendWith"), [this]
    {
        It(TEXT("将当前Transform与目标Transform混合"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Transform1 = UE4.FTransform(UE4.FQuat(0,0,0,1), UE4.FVector(0,0,1))\
            local Transform2 = UE4.FTransform(UE4.FQuat(0,0,0,1), UE4.FVector(0,0,1) * 2)\
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
        It(TEXT("将当前Transform与目标两个Transform混合"), EAsyncExecution::ThreadPool, [this]()
        {
            const char* Chunk = "\
            local Transform1 = UE4.FTransform(UE4.FQuat(0,0,0,1), UE4.FVector(0,0,1))\
            local Transform2 = UE4.FTransform(UE4.FQuat(0,0,0,1), UE4.FVector(0,0,2))\
            local Transform3 = UE4.FTransform(UE4.FQuat(0,0,0,1), UE4.FVector(0,0,3))\
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
        It(TEXT("转为字符串"), EAsyncExecution::ThreadPool, [this]()
        {
            UnLua::RunChunk(L, "return tostring(UE4.FTransform())");
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
