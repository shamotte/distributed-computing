#include "state.h"

StateIdle::StateIdle(Context *_ctx)
{
    ctx = _ctx;
}
extern std::string global_state_name;

void StateIdle::Logic()
{

    global_state_name = "IDLE";
    ctx->table_number = -1;
    ctx->companions.clear();
    ctx->chosen_game = -1;

    coutcolor("zmienilem stan na IDLE");

    randSleep();

    coutcolor("Zacznę szukać...");

    ctx->next_state = STATE_SEEK;
    return;
}