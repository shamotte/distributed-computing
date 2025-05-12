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

extern int RANK, SIZE;
template <typename... Args>
void coutcolor(Args &&...args)
{
    std::ostringstream oss;
    (oss << ... << args); // składnia fold expression (C++17)
    std::cout << "\033[0;" << (31 + RANK % 7) << "m"
              << oss.str()
              << "\033[0m\n";
}

#pragma region BaseState

void BaseState::Logic()
{
}

void BaseState::ProcessState(Datatype &d)
{

    switch (d.type)
    {

    case SIG_TABLE_REQ:

    case SIG_TABLE_ACK:

    case SIG_TABLE:

    case SIG_END_REQ:

    case SIG_GAME_END:
        break;
    }
}

void BaseState::ProcessSIG_TABLE_REQ(Datatype &d)
{

    // signal SIG_TABLE_REQ(priority: int, vote: int) {
    //     queue.insert(
    //         queue.find(pririty, signal.pid) // Znajdź miejsce w kolejce na bazie priorytetu, ewentualnie pid
    //         { signal.pid, priority, vote }
    //     )
    //     players_acknowledged[tabel_req] = true
    //     send(SIG_TABLE_ACK)
    // }
}

void BaseState::ProcessSIG_SIG_TABLE_ACK(Datatype &d)
{
}

void BaseState::ProcessSIG_TABLE(Datatype &d)
{
}

void BaseState::ProcessSIG_END_REQ(Datatype &d)
{
}

void BaseState::ProcessSIG_GAME_END(Datatype &d)
{
}

#pragma endregion

#pragma region Idle

StateIdle::StateIdle(Context *ctx)
{
    ctx = ctx;
}

void StateIdle::Logic()
{
    coutcolor("zmienilem stan na IDLE");
    ctx->priority = ctx->lamport;
    std::this_thread::sleep_for(std::chrono::seconds(rand() % 10));
    ctx->next_state = STATE_SEEK;
}

#pragma endregion

#pragma region seek
StateSeek::StateSeek(Context *ctx)
{
    ctx = ctx;
}

void StateSeek::Logic()
{
    coutcolor("zmienilem stan na SEEK");
}

#pragma endregion

#pragma region Play

StatePlay::StatePlay(Context *ctx)
{
    ctx = ctx;
}

void StatePlay::Logic()
{
    coutcolor("zmienilem stan na PLAY");
}

#pragma endregion

#pragma region Utils
BaseState *Context::GetNextState()
{
    coutcolor("next state is", next_state);
    return States[next_state];
}

#pragma endregion