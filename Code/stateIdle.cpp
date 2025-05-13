#include "state.h"

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