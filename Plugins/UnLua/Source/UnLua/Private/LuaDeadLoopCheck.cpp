// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaDeadLoopCheck.h"
#include "lauxlib.h"
#include "HAL/RunnableThread.h"
#include "UnLuaModule.h"

const int MaxLuaExecTime = 5; // in second

FDeadLoopCheck::FDeadLoopCheck()
    : timeoutEvent(nullptr)
    , timeoutCounter(0)
    , stopCounter(0)
    , frameCounter(0)
{
    thread = FRunnableThread::Create(this, TEXT("FLuaDeadLoopCheck"), 0, TPri_BelowNormal);
}

FDeadLoopCheck::~FDeadLoopCheck()
{
    Stop();
    thread->WaitForCompletion();
    delete thread;
    thread = nullptr;
}

uint32 FDeadLoopCheck::Run()
{
    while (stopCounter.GetValue() == 0)
    {
        FPlatformProcess::Sleep(1.0f);
        if (frameCounter.GetValue() != 0)
        {
            timeoutCounter.Increment();
            if (timeoutCounter.GetValue() >= MaxLuaExecTime)
            {
                onScriptTimeout();
            }
        }
    }
    return 0;
}

void FDeadLoopCheck::Stop()
{
    stopCounter.Increment();
}

int FDeadLoopCheck::EnterScript(ScriptTimeoutEvent* pEvent)
{
    int ret = frameCounter.Increment();
    if (ret == 1)
    {
        timeoutCounter.Set(0);
        timeoutEvent.store(pEvent);
    }
    return ret;
}

int FDeadLoopCheck::LeaveScript()
{
    return frameCounter.Decrement();
}

void FDeadLoopCheck::onScriptTimeout()
{
    auto pEvent = timeoutEvent.load();
    if (pEvent)
    {
        timeoutEvent.store(nullptr);
        pEvent->OnTimeout();
    }
}

LuaScriptCallGuard::LuaScriptCallGuard(lua_State* L_)
    :L(L_)
{
    const auto Env = IUnLuaModule::Get().GetEnv();
    if (Env)
    {
        Env->GetDeadLoopCheck()->EnterScript(this);
    }
}

LuaScriptCallGuard::~LuaScriptCallGuard()
{
    const auto Env = IUnLuaModule::Get().GetEnv();
    if (Env)
    {
        Env->GetDeadLoopCheck()->LeaveScript();
    }
}

void LuaScriptCallGuard::OnTimeout()
{
    auto hook = lua_gethook(L);
    if (hook == nullptr)
    {
        lua_sethook(L, scriptTimeout, LUA_MASKLINE, 0);
    }
}

void LuaScriptCallGuard::scriptTimeout(lua_State* L, lua_Debug* ar)
{
    lua_sethook(L, nullptr, 0, 0);
    luaL_error(L, "lua script exec timeout");
}
