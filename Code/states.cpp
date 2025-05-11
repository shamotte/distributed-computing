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

void BaseState::EnterState()
{
    ctx->logic_thread = std::thread(BaseState::Logic, this);
}

void BaseState::Logic()
{
}

void BaseState::ProcessState()
{
}

void BaseState::changeState(State new_state)
{
    std::unique_lock(ctx->state_mutex);

    ctx->current_state = ctx->States[new_state];
}