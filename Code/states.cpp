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
#include "signalSender.h"

#include <algorithm>

extern int RANK, SIZE;
extern unsigned int global_lamport;
extern unsigned int games_played;

#pragma region BaseState

void BaseState::Logic()
{
}



#pragma endregion

#pragma region

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

    std::this_thread::sleep_for(std::chrono::seconds((MAX_SLEEP > 0) ? rand() % MAX_SLEEP : MAX_SLEEP));
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
    ctx->priority = global_lamport;
    coutcolor("zmienilem stan na SEEK (priority: ", ctx->priority, ")");

    Broadcast_SIG_TABLE_REQ(ctx->priority, rand() % GAME_NUM);

    std::mutex x;
    std::unique_lock lock(x);

    ctx->cv_seek.wait(lock, [this]()
                      { 
                        /*std::stringstream ss;
                        ss<<"M: "<< (ctx->priority)<<" , ";
                        for(int b : ctx->players_acknowledged)
                        {
                            ss<<b<<" ";
                        }
                        coutcolor(ss.str());*/

                        return std::all_of(this->ctx->players_acknowledged,
                                        this->ctx->players_acknowledged + PLAYER_NUM,
                                        [this](int b)
                                        { return ctx->priority < b; }); });

    coutcolor(RANK, " Otrzymałem odpowiedzi!");

    while (true)
    {

        coutcolor("----- SPRAWDZAM WEJŚCIE -----");

        auto &queue = ctx->queue;

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

        for (int pos = 0; pos < queue.size(); pos += SEAT_COUNT)
        {
            int table_index = pos / SEAT_COUNT;
            if (pos + SEAT_COUNT > queue.size())
            {
                // coutcolor(RANK, " nie ma wystarczającej liczby graczy");
                break;
            }

            // coutcolor(RANK, "stół o indeksie", table_index, "zawiera graczy", queue[pos].pid, queue[pos + 1].pid, queue[pos + 2].pid);

            int first_player_index = table_index * SEAT_COUNT;
            int last_player_index = table_index * SEAT_COUNT + SEAT_COUNT - 1;

            bool is_last = RANK == queue[last_player_index].pid;
            // coutcolor(RANK, "jestem ostatni? (", queue[last_player_index].pid, ")", is_last);
            if (is_last)
            {

                // Obierz stół
                for (auto tid : ctx->table_numbers)
                {
                    coutcolor("Inicjuję grę przy stole: ", tid);
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
                coutcolor("PLAY INITIALIZER");
                return;
            }
        }

        // Nie znaleziono stołu
        coutcolor("Nie znaleziono.");

        // Zakończ stan jeśli ustawiono na PLAY - failsafe
        if (ctx->next_state == STATE_PLAY)
        {
            coutcolor("PLAY 1");
            return;
        }

        ctx->cv_seek_wake.wait(lock);

        // Zakończ stan jeśli ustawiono na PLAY
        if (ctx->next_state == STATE_PLAY)
        {
            coutcolor("PLAY 2");
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
    std::stringstream ss;
    for (auto c: ctx->companions) {
        ss << c << ",";
    }
    coutcolor("zmienilem stan na PLAY - ", ss.str());

    std::this_thread::sleep_for(std::chrono::seconds((MAX_SLEEP > 0) ? rand() % MAX_SLEEP : MAX_SLEEP));

    for (auto comp : ctx->companions)
    {
        Send_SIG_END_REQ(comp);
    }

    std::mutex x;
    std::unique_lock lock(x);

    if (!ctx->end_ready == SEAT_COUNT) { // Edge case kiedy nie zdąży dojść 
        ctx->cv_game_end_req.wait(lock, [this](){
            coutcolor("obudzoned");
            return this->ctx->end_ready == SEAT_COUNT;
        });
    }

    coutcolor("Wszyscy gotowi do zakończenia!");

    if (RANK == *std::min_element(ctx->companions.begin(), ctx->companions.end()))
    {
        std::stringstream ss;
        for (auto c : ctx->companions)
        {
            ss << c << ",";
        }

        coutcolor("GAME OVER, GO HOME.", ss.str());
        Broadcast_SIG_GAME_END(ctx->companions, ctx->table_number);
    }

    ctx->cv_gameover.wait(lock, [this]()
        { return this->ctx->next_state == STATE_IDLE; }
    );

    coutcolor("Gra zakończona!");
    games_played++;

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