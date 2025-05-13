#include "state.h"

StatePlay::StatePlay(Context *_ctx)
{
    ctx = _ctx;
}
extern std::string global_state_name;
void StatePlay::Logic()
{
    global_state_name = "PLAY";
    std::stringstream ss;
    for (auto c : ctx->companions)
    {
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

    if (!ctx->end_ready == SEAT_COUNT)
    { // Edge case kiedy nie zdąży dojść
        ctx->cv_game_end_req.wait(lock, [this]()
                                  {
            coutcolor("obudzoned");
            return this->ctx->end_ready == SEAT_COUNT; });
    }
    ctx->end_ready = 0;
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
                          { return this->ctx->cv_game_over_flag; });

    ctx->cv_game_over_flag = false;
    coutcolor("Gra zakończona!");
    games_played++;

    ctx->next_state = STATE_IDLE;
    return;
}