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
#include "states.h"
#include "signals.h"

#include <algorithm>

extern int RANK, SIZE;
extern unsigned int global_lamport;

#pragma region BaseState

void BaseState::Logic()
{
}

void BaseState::ProcessSignal(Datatype &d)
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
}

void BaseState::ProcessSIG_TABLE_REQ(Datatype &d)
{
    std::vector<QueuePosition> &queue = ctx->queue;
    ctx->queue.insert(
        std::find_if(queue.begin(), queue.end(), [&d](QueuePosition p)
                     { return d.priority * SIZE + d.pid < p.priority * SIZE + p.pid; }),
        QueuePosition{d.pid, d.priority, d.vote});

    ctx->players_acknowledged[d.pid] = d.lamport;

    ctx->cv_seek.notify_all();

    Send_SIG_SIG_TABLE_ACK(d.pid);
}

void BaseState::ProcessSIG_SIG_TABLE_ACK(Datatype &d)
{
    ctx->players_acknowledged[d.pid] = d.lamport;
    ctx->cv_seek.notify_all();
}

void BaseState::ProcessSIG_TABLE(Datatype &d)
{
    ctx->table_number = d.table_number;
    ctx->chosen_game = d.chosen_game;

    ctx->companions.clear();
    ctx->companions = std::set<int>(
        d.players,
        d.players + SEAT_COUNT);

    ctx->next_state = STATE_PLAY;
    ctx->cv_seek_wake.notify_all();
}

void BaseState::ProcessSIG_END_REQ(Datatype &d)
{
    ctx->end_ready++;
    ctx->cv_game_end_req.notify_all();
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

        // HUH?
        ////ctx->players_acknowledged[player] = false;
    }

    std::vector<int> &tables = ctx->table_numbers;

    std::remove_if(tables.begin(), tables.end(), [&d](int t)
                   { return d.table_number == t; }); // przesuwamy właśnie zwolniony stół na koniec kolejki

    ctx->cv_seek_wake.notify_all();
}

#pragma endregion

#pragma region Idle

StateIdle::StateIdle(Context *_ctx)
{
    ctx = _ctx;
}

void StateIdle::Logic()
{

    ctx->table_number = -1;
    ctx->companions.clear();
    ctx->end_ready = 0;
    ctx->chosen_game = -1;

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
                        ss<<"M: "<< (ctx->priority)<<" , ";
                        for(bool b : ctx->players_acknowledged)
                        {
                            ss<<b<<" ";
                        }
                        coutcolor(ss.str());

                        return std::all_of(this->ctx->players_acknowledged,
                                        this->ctx->players_acknowledged + PLAYER_NUM,
                                        [this](int b)
                                        { return ctx->priority < b; }); });

    coutcolor(RANK, " wlaskie zakonczyl czekanie");

    while (true)
    {

        auto &queue = ctx->queue;
        for (int pos = 0; pos < queue.size(); pos += SEAT_COUNT)
        {
            int table_index = pos / SEAT_COUNT;
            if (pos + SEAT_COUNT > queue.size())
            {
                coutcolor(RANK, " nie ma wystarczającej liczby graczy");
                break;
            }

            coutcolor(RANK, "stół o indeksie", table_index, "zawiera graczy", queue[pos].pid, queue[pos + 1].pid, queue[pos + 2].pid);

            int first_player_index = table_index * SEAT_COUNT;
            int last_player_index = table_index * SEAT_COUNT + SEAT_COUNT - 1;

            bool is_last = RANK == queue[last_player_index].pid;
            coutcolor(RANK, "jestem ostatni? (", queue[last_player_index].pid, ")", is_last);
            if (is_last)
            {

                // Obierz stół
                for (auto tid : ctx->table_numbers)
                {
                    coutcolor("stół", tid);
                }
                ctx->table_number = ctx->table_numbers[table_index];

                // Wykryj współgraczy
                ctx->companions.clear();
                for (int i = first_player_index; i <= last_player_index; i++)
                {
                    ctx->companions.insert(queue[i].pid);
                }

                // Zlicz głosy
                int votes[SEAT_COUNT] = {};
                for (int i = table_index * SEAT_COUNT; i < table_index * SEAT_COUNT + SEAT_COUNT; i++)
                {
                    votes[queue[i].vote]++;
                }

                // Indeks o największej liczbie głosów
                int chosen_game = std::max_element(votes, votes + GAME_NUM) - votes;
                ctx->chosen_game = chosen_game;

                // Wysłanie SIG_TABLE
                for (auto comp : ctx->companions)
                {
                    if (comp != RANK)
                    {
                        Send_SIG_TABLE(comp, ctx->companions, ctx->table_number, chosen_game);
                    }
                }

                // Własne przejście do stanu gry
                ctx->next_state = STATE_PLAY;
                return;
            }
        }

        // Nie znaleziono stołu
        coutcolor(RANK, " nie znaleziiono stołu");

        // Zakończ stan jeśli ustawiono na PLAY - failsafe
        if (ctx->next_state == STATE_PLAY)
        {
            return;
        }

        ctx->cv_seek_wake.wait(lock);

        // Zakończ stan jeśli ustawiono na PLAY
        if (ctx->next_state == STATE_PLAY)
        {
            return;
        }
    }
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

    std::this_thread::sleep_for(std::chrono::seconds(rand() % 10));

    for (auto comp : ctx->companions)
    {
        Send_SIG_END_REQ(comp);
    }

    std::mutex x;
    std::unique_lock lock(x);

    ctx->cv_game_end_req.wait(lock, [this]()
                              { return this->ctx->end_ready == SEAT_COUNT; });

    coutcolor("Gra zakończona!");

    if (RANK == *std::min_element(ctx->companions.begin(), ctx->companions.end()))
    {
        coutcolor("GAME OVER, GO HOME.");
        Broadcast_SIG_GAME_END(ctx->companions, ctx->table_number);
    }

    ctx->next_state = STATE_IDLE;
    return;
}

#pragma endregion

#pragma region Utils
BaseState *Context::GetNextState()
{
    coutcolor("next state is", priority);
    return States[next_state];
}

#pragma endregion