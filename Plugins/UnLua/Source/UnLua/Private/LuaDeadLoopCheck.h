// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "lua.h"
#include <atomic>
#include "HAL/Runnable.h"
#include "Tickable.h"

struct ScriptTimeoutEvent
{
    virtual void OnTimeout() = 0;
};

class FDeadLoopCheck : public FRunnable
{
public:
    FDeadLoopCheck();
    ~FDeadLoopCheck();

    int EnterScript(ScriptTimeoutEvent* pEvent);
    int LeaveScript();

protected:
    uint32 Run() override;
    void Stop() override;
    void onScriptTimeout();

private:
    std::atomic<ScriptTimeoutEvent*> timeoutEvent;
    FThreadSafeCounter timeoutCounter;
    FThreadSafeCounter stopCounter;
    FThreadSafeCounter frameCounter;
    FRunnableThread* thread;
};

class LuaScriptCallGuard : public ScriptTimeoutEvent
{
public:
    LuaScriptCallGuard(lua_State* L);
    virtual ~LuaScriptCallGuard();
    void OnTimeout() override;

private:
    lua_State* L;
    static void scriptTimeout(lua_State* L, lua_Debug* ar);
};
