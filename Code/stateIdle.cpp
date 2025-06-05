#include "state.h"

StateIdle::StateIdle(Context *_ctx)
{
    ctx = _ctx;
}
extern std::string global_state_name;

void StateIdle::Logic()
{
    {
        std::unique_lock lock(ctx->global_mutex);
        global_state_name = "IDLE";
        ctx->table_number = -1;
        ctx->companions.clear();
        ctx->chosen_game = -1;

        coutcolor("zmienilem stan na IDLE");
    }

    randSleep();

    {
        std::unique_lock lock(ctx->global_mutex);
        coutcolor("Zacznę szukać...");

        ctx->next_state = STATE_SEEK;
    }

    return;
}