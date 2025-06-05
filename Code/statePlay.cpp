#include "state.h"

extern std::string global_state_name;
void StatePlayLogic(Context *ctx)
{coutcolor("AAAAAAA");
    {
        std::unique_lock lock(ctx->global_mutex);

        global_state_name = "PLAY";
        std::stringstream ss;
        for (auto c : ctx->companions)
        {
            ss << c << ",";
        }
        coutcolor("zmienilem stan na PLAY - ", ss.str());
        coutcolor("Gram w grę ", ctx->chosen_game, " przy stole ", ctx->table_number);
    }

    randSleep();

    {
        std::unique_lock lock(ctx->global_mutex);

        coutcolor("Zgłaszam gotowość do zakończenia!");

        for (auto comp : ctx->companions)
        {
            Send_SIG_END_REQ(comp);
        }



        global_state_name = "PLAY:WAIT";

        ctx->cv_game_end_req.wait(lock, [ctx]() {
            if (DEBUG) {
                coutcolor("obudzoned ", ctx->end_ready);
            }
            return ctx->end_ready >= SEAT_COUNT; 
        });

        ctx->end_ready = 0;
        coutcolor("Wszyscy gotowi do zakończenia!");


        // Inicjator końca
        if (RANK == *std::min_element(ctx->companions.begin(), ctx->companions.end())) {
            std::stringstream ss;
            for (auto c : ctx->companions)
            {
                ss << c << ",";
            }

            coutcolor("Ogłaszam koniec gry przy stole: ", ctx->table_number, " (", ss.str(), ")");
            Broadcast_SIG_GAME_END(ctx->companions, ctx->table_number);
        }


        // Czekanie na koniec
        if (DEBUG) {
            coutcolor("GAME OVER FLAG = ", ctx->cv_game_over_flag);
        }

        global_state_name = "PLAY:END";
        {
            ctx->cv_gameover.wait(lock, [ctx](){
                return ctx->cv_game_over_flag;
            });
        }

        ctx->cv_game_over_flag = false;

        if (DEBUG) {
            coutcolor("flaga ustawiona na ", ctx->cv_game_over_flag);
        }

        coutcolor("Gra zakończona! Stół: ", ctx->table_number);
        games_played++;

        ctx->next_state = STATE_IDLE;
        return;
    }
}