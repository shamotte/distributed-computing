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
extern unsigned int global_lamport;
extern unsigned int games_played;


void BaseState::Logic() {}


BaseState *Context::GetNextState()
{
    coutcolor("next state is", priority);
    return States[next_state];
}
