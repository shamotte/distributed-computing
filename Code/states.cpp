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
#include <algorithm>

extern int RANK, SIZE;
extern int global_lamport;

template <typename... Args>
void coutcolor(Args &&...args)
{
    std::ostringstream oss;
    (oss << ... << args); // składnia fold expression (C++17)
    std::cout << "\033[0;" << (31 + RANK % 7) << "m"
              << "["
              << global_lamport
              << "\t]\t"
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
        ProcessSIG_TABLE_REQ(d);
        break;
    case SIG_TABLE_ACK:
        ProcessSIG_SIG_TABLE_ACK(d);
        break;
    case SIG_TABLE:
        ProcessSIG_END_REQ(d);
        break;
    case SIG_END_REQ:
        ProcessSIG_END_REQ(d);
        break;
    case SIG_GAME_END:
        ProcessSIG_GAME_END(d);
        break;
    }
}

void BaseState::ProcessSIG_TABLE_REQ(Datatype &d)
{
    std::vector<QueuePosition> &queue = ctx->queue;
    ctx->queue.insert(
        std::find_if(queue.begin(), queue.end(), [&d](QueuePosition p)
                     { return d.priority * SIZE + d.pid < p.priority * SIZE + p.pid; }),
        (QueuePosition){d.pid, d.priority, d.vote});

    ctx->players_acknowledged[d.pid] = true;

    ctx->cv_seek.notify_all();
    Send_SIG_SIG_TABLE_ACK(d.pid);
}

void BaseState::ProcessSIG_SIG_TABLE_ACK(Datatype &d)
{
    ctx->players_acknowledged[d.pid] = true;
    ctx->cv_seek.notify_all();
}

void BaseState::ProcessSIG_TABLE(Datatype &d)
{
}

void BaseState::ProcessSIG_END_REQ(Datatype &d)
{
    ctx->end_ready++;
}

void BaseState::ProcessSIG_GAME_END(Datatype &d)
{
    std::set<int> companions(d.players, d.players + PLAYER_NUM);

    std::vector<QueuePosition> &queue = ctx->queue;
    for (auto player : d.players)
    {
        queue.erase(
            std::remove_if(queue.begin(),
                           queue.end(),
                           [&companions](QueuePosition p)
                           { return companions.find(p.pid) != companions.end(); }),
            queue.end()); // usuwamy gaczy z kolejki

        ctx->players_acknowledged[player] = false;
    }

    std::vector<int> &tables = ctx->table_numbers;

    std::remove_if(tables.begin(), tables.end(), [&d](int t)
                   { return d.table_number == t; }); // przesuwamy właśnie zwolniony stół na koniec kolejki
}

#pragma endregion

#pragma region Idle

StateIdle::StateIdle(Context *_ctx)
{
    ctx = _ctx;
}

void StateIdle::Logic()
{
    coutcolor("zmienilem stan na IDLE");
    ctx->priority = global_lamport;
    std::this_thread::sleep_for(std::chrono::seconds(rand() % 10));
    ctx->next_state = STATE_SEEK;
    return;
}

#pragma endregion

#pragma region seek
StateSeek::StateSeek(Context *_ctx)
{
    ctx = _ctx;
}

void StateSeek::Logic()
{
    coutcolor("zmienilem stan na SEEK");

    Broadcast_SIG_TABLE_REQ(ctx->priority, rand() % GAME_NUM);
    std::mutex x;
    std::unique_lock lock(x);

    ctx->cv_seek.wait(lock, [this]()
                      { 
                        std::stringstream ss;
                        for(bool b : ctx->players_acknowledged)
                        {
                            ss<<b<<" ";
                        }
                        coutcolor(ss.str());

                        return std::all_of(this->ctx->players_acknowledged,
                                           this->ctx->players_acknowledged + PLAYER_NUM,
                                           [](bool b)
                                           { return b; }); });

    coutcolor(RANK, " wlaskie zakonczyl czekanie");
    while (true)
        ;
}

#pragma endregion

#pragma region Play

StatePlay::StatePlay(Context *_ctx)
{
    ctx = _ctx;
}

void StatePlay::Logic()
{
    coutcolor("zmienilem stan na PLAY");
}

#pragma endregion

#pragma region Utils
BaseState *Context::GetNextState()
{
    coutcolor("next state is", priority);
    return States[next_state];
}

#pragma endregion