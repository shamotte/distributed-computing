#include "state.h"

extern std::mutex pls_work;

StateSeek::StateSeek(Context *_ctx)
{
    ctx = _ctx;
}
extern std::string global_state_name;
void StateSeek::Logic()
{
    global_state_name = "SEEK";
    {
        std::unique_lock lock(pls_work);

        ctx->priority = global_lamport;
        coutcolor("zmienilem stan na SEEK (priority: ", ctx->priority, ")");

        Broadcast_SIG_TABLE_REQ(ctx->priority, rand() % GAME_NUM);
    }

    std::unique_lock lock(ctx->mt_seek);

    ctx->cv_seek.wait(lock, [this]()
                      { 
		std::stringstream ss;
		ss<<"M: "<< (ctx->priority)<<" , ";
		for(int b : ctx->players_acknowledged) {
			ss<<b<<" ";
		}
		coutcolor(ss.str());


		return std::all_of(
			this->ctx->players_acknowledged,
			this->ctx->players_acknowledged + PLAYER_NUM,
			[this](int b){ return ctx->priority < b; }
		); });

    coutcolor(" Otrzymałem odpowiedzi!");

    ctx->cv_new_table_req_flag = false;
    while (true)
    {

        coutcolor("----- SPRAWDZAM WEJŚCIE -----");

        {
            std::unique_lock queuelock(ctx->mt_queue);
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
        }

        // Nie znaleziono stołu
        coutcolor("Nie znaleziono.");

        // Zakończ stan jeśli ustawiono na PLAY - failsafe
        if (ctx->next_state == STATE_PLAY)
        {
            coutcolor("PLAY 1");
            return;
        }

        ctx->cv_seek_wake.wait(lock, [this]()
                               { return this->ctx->cv_new_table_req_flag; });
        ctx->cv_new_table_req_flag = false;

        // Zakończ stan jeśli ustawiono na PLAY
        if (ctx->next_state == STATE_PLAY)
        {
            coutcolor("PLAY 2");
            return;
        }
    }
}