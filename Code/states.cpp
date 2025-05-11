#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <chrono>

#include "config.h"
#include "functions.h"
#include "states.h"

enum State
{
    STATE_IDLE,
    STATE_SEEK,
    STATE_PLAY
};

class BaseState
{

public:
    Context *ctx;

    virtual void Logic();
    virtual void ProcessSignal();

    void EnterState()
    {
        ctx->logic_thread = std::thread(BaseState::Logic, this);
    }

    void changeState(State new_state)
    {
        std::unique_lock(ctx->state_mutex);
    }
};

class StateIdle : BaseState
{
public:
    void Logic()
    {
        coutcolor("process zaczął czekanie");
        std::chrono::seconds time_to_sleep(rand() % 10);
        std::this_thread::sleep_for(time_to_sleep);
        changeState(STATE_SEEK);
    }

    void ProcessSignal()
    {
    }
};

class StateSeek : BaseState
{
};

class StatePlay : BaseState
{
};
