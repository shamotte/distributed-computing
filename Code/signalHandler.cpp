#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <chrono>
#include <stdlib.h>
#include <cstring>

#include "config.h"
#include "functions.h"
#include "state.h"
#include "signalSender.h"

#include <algorithm>

extern int RANK, SIZE;
extern volatile unsigned int global_lamport;
extern unsigned int games_played;

void BaseState::ProcessSignal(MPIMessage &d)
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
        ProcessSIG_TABLE(d);
        break;
    case SIG_END_REQ:
        ProcessSIG_END_REQ(d);
        break;
    case SIG_GAME_END:
        ProcessSIG_GAME_END(d);
        break;
    }

    auto &queue = ctx->queue;

    std::stringstream ss;
    ss << "STAN KOLEJKI: ";
    for (int pos = 0; pos < queue.size(); pos += 1)
    {
        ss << queue[pos].pid << "(" << queue[pos].priority << ")" << " ";
        if ((pos + 1) % SEAT_COUNT == 0)
        {
            ss << "|";
        }
    }
    coutcolor(ss.str());
}

void BaseState::ProcessSIG_TABLE_REQ(MPIMessage &d)
{
    std::unique_lock lock(ctx->mt_seek);

    std::vector<QueuePosition> &queue = ctx->queue;
    ctx->queue.insert(
        std::find_if(queue.begin(), queue.end(), [&d](QueuePosition p)
                     { return d.priority * SIZE + d.pid < p.priority * SIZE + p.pid; }),
        QueuePosition{d.pid, d.priority, d.vote});

    std::map<int, int> occurances;
    for (QueuePosition i : queue)
    {
        occurances[i.pid]++;
    }

    for (auto x : occurances)
    {
        if (x.second > 1)
        {
            coutcolor("PODWUJNA LICBA W KOLEJCE");
        }
    }
    std::stringstream ss;
    ss << "STAN KOLEJKI: ";
    for (int pos = 0; pos < queue.size(); pos += 1)
    {
        ss << queue[pos].pid << " ";
        if ((pos + 1) % SEAT_COUNT == 0)
        {
            ss << "|";
        }
    }
    coutcolor(ss.str());

    ctx->players_acknowledged[d.pid] = std::max(d.lamport, ctx->players_acknowledged[d.pid]);

    ctx->cv_seek.notify_all();
    ctx->cv_seek_wake.notify_all();
    ctx->cv_new_table_req_flag = true;

    Send_SIG_SIG_TABLE_ACK(d.pid);
}

void BaseState::ProcessSIG_SIG_TABLE_ACK(MPIMessage &d)
{
    std::unique_lock lock(ctx->mt_seek);
    ctx->players_acknowledged[d.pid] = std::max(d.lamport, ctx->players_acknowledged[d.pid]);
    ctx->cv_seek.notify_all();
    ctx->cv_new_table_req_flag = true;
}

void BaseState::ProcessSIG_TABLE(MPIMessage &d)
{

    std::unique_lock lock(ctx->mt_seek);

    ctx->table_number = d.table_number;
    ctx->chosen_game = d.chosen_game;

    ctx->companions.clear();
    ctx->companions = std::set<int>(
        d.players,
        d.players + SEAT_COUNT);

    ctx->cv_new_table_req_flag = true;
    ctx->next_state = STATE_PLAY;
    ctx->cv_seek_wake.notify_all();
}

void BaseState::ProcessSIG_END_REQ(MPIMessage &d)
{
    std::unique_lock lock(ctx->mt_play_end_req);
    ctx->end_ready++;
    coutcolor("Ilość gotowych do zakończenia: ", ctx->end_ready);
    ctx->cv_game_end_req.notify_all();
}

void BaseState::ProcessSIG_GAME_END(MPIMessage &d)
{
    std::set<int> companions(d.players, d.players + SEAT_COUNT);

    std::stringstream ss;
    ss << "do usuniecia";
    for (int x : companions)
    {
        ss << x << " ";
    }
    coutcolor(ss.str());

    std::vector<QueuePosition> &queue = ctx->queue;
    // queue.erase(
    //     std::remove_if(queue.begin(),
    //                    queue.end(),
    //                    [&companions, &d](QueuePosition p)
    //                    { return (companions.find(p.pid) != companions.end()) && (p.priority < d.lamport); }),
    //     queue.end()); // usuwamy gaczy z kolejki

    for (int companion : companions)
    {
        queue.erase(std::find_if(queue.begin(), queue.end(), [companion, &d](QueuePosition pos)
                                 { return (pos.pid == companion) && (pos.priority < d.lamport); }));
    }

    std::vector<int> &tables = ctx->table_numbers;

    std::remove_if(tables.begin(), tables.end(), [&d](int t)
                   { return d.table_number == t; }); // przesuwamy właśnie zwolniony stół na koniec kolejki

    if (ctx->current_state != ctx->States[STATE_IDLE])
    {
        if (std::find_if(queue.begin(), queue.end(), [](QueuePosition pos)
                         { return pos.pid == RANK; }) == queue.end())
        {
            coutcolor("SELF NOT IN QUEUE");
        }
    }

    ctx->cv_seek_wake.notify_all();
    ctx->cv_game_end_req.notify_all();

    if (d.table_number == ctx->table_number)
    {
        ctx->cv_game_over_flag = true;
        coutcolor("flaga ustawiona na ", ctx->cv_game_over_flag);
        ctx->cv_gameover.notify_all();
    }
}