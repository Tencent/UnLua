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

#include "TestCommands.h"
#include "UnLuaTestCommon.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_TESTSUITE(FIssue372Test, TEXT("UnLua.Regression.Issue372 Actor调用K2_DestroyActor销毁时，lua部分会在注册表残留"));

    bool FIssue372Test::RunTest(const FString& Parameters)
    {
        const auto MapName = TEXT("/UnLuaTestSuite/Tests/Regression/Issue372/Issue372");
        ADD_LATENT_AUTOMATION_COMMAND(FOpenMapLatentCommand(MapName))
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this] {
            const auto L = UnLua::GetState();
            UnLua::PushUObject(L, GWorld);
            lua_setglobal(L, "World");

            UnLua::PushUObject(L, GWorld);

            const auto Chunk = R"(
            local ActorClass = UE.UClass.Load("/UnLuaTestSuite/Tests/Binding/BP_UnLuaTestActor_StaticBinding.BP_UnLuaTestActor_StaticBinding_C")
            Actor = World:SpawnActor(ActorClass)

            local registry = debug.getregistry()
            for k, v in pairs(registry) do
                if v == Actor then
                    return k
                end
            end
            return -2 -- LUA_NOREF
            )";
            UnLua::RunChunk(L, Chunk);

            auto Ref = (int32)lua_tointeger(L, -1);
            TestNotEqual(TEXT("Ref"), Ref, LUA_NOREF);

            lua_rawgeti(L, LUA_REGISTRYINDEX, Ref);
            auto Actual = lua_type(L, -1);
            TestEqual(TEXT("Actual"), Actual, LUA_TTABLE);

            UnLua::RunChunk(L, "Actor:K2_DestroyActor()");
            CollectGarbage(RF_NoFlags, true);

            lua_rawgeti(L, LUA_REGISTRYINDEX, Ref);
            Actual = lua_type(L, -1);
            TestNotEqual(TEXT("Actual"), Actual, LUA_TTABLE);
            return true;
            }));
        ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
        return true;
    }

END_TESTSUITE(FRegression_Issue276)

#endif
