#include "state.h"

extern std::string global_state_name;
void StateSeekLogic(Context *ctx)
{
    {
        std::unique_lock lock(ctx->global_mutex);

        global_state_name = "SEEK";

        ctx->priority = global_lamport;
        coutcolor("zmienilem stan na SEEK (priority: ", ctx->priority, ")");

        int game = rand() % GAME_NUM;
        coutcolor(" Glosuje na: ", game);
        Broadcast_SIG_TABLE_REQ(ctx->priority, game);

        coutcolor("Czekam na odpowiedzi innych...");
    }

    randSleep();

    {
        std::unique_lock lock(ctx->global_mutex);

        ctx->cv_seek.wait(lock, [this]() { 

            if (DEBUG) {
                std::stringstream ss;
                ss<<"M: "<< (ctx->priority)<<" , ";
                for(int b : ctx->players_acknowledged) {
                    ss<<b<<" ";
                }
                coutcolor(ss.str());
            }


            return std::all_of(
                this->ctx->players_acknowledged,
                this->ctx->players_acknowledged + PLAYER_NUM,
                [this](int b){ return ctx->priority < b; }
            );
        });

        coutcolor(" Otrzymałem odpowiedzi!");

        ctx->cv_new_table_req_flag = false;
        while (true)
        {

            if (DEBUG) {
                coutcolor("----- SPRAWDZAM WEJŚCIE -----");
            }

            auto &queue = ctx->queue;

            if (DEBUG) {

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
            }

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

                    // Print
                    std::stringstream ss;
                    for (auto c : ctx->companions)
                    {
                        ss << c << ",";
                    }
                    coutcolor("Inicjuję grę przy stole: ", ctx->table_number, " (", ss.str(), ") gra: ", ctx->chosen_game);

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
                    coutcolor("Inicjalizacja gotowa, zaczynam grę!");
                    return;
                }
            }

            // Nie znaleziono stołu
            coutcolor("Nie znaleziono stołu - czekam.");

            // Zakończ stan jeśli ustawiono na PLAY - failsafe
            if (ctx->cv_new_table_req_flag)
            {
                ctx->next_state == STATE_PLAY;
                coutcolor("Rozpoczynam grę przy stole: ", ctx->table_number);
                ctx->cv_new_table_req_flag = false;
                return;
            }

            ctx->cv_seek_wake.wait(lock, [this]() {
                return this->ctx->cv_new_table_req_flag;
            });

            // Zakończ stan jeśli ustawiono na PLAY
            if (ctx->cv_new_table_req_flag)
            {
                ctx->next_state == STATE_PLAY;
                coutcolor("Rozpoczynam grę przy stole: ", ctx->table_number);
                ctx->cv_new_table_req_flag = false;
                return;
            }
        }
    }
}