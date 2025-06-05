#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <sstream>
#include <set>
#include <chrono>
#include <map>
#include <algorithm>

#include "config.h"
#include "functions.h"
#include <condition_variable>

#include "signalSender.h"

enum State
{
    STATE_IDLE,
    STATE_SEEK,
    STATE_PLAY,
    STATE_STAY
};
const std::string StateNames[] = {
    "IDLE",
    "SEEK",
    "PLAY",
    "STAY"};

struct QueuePosition
{
    unsigned int pid;
    int priority;
    int vote;
};


class Context
{
public:
    std::mutex global_mutex;

    State current_state;
    State next_state;

    int priority = 0;
    std::condition_variable cv_seek;
    std::condition_variable cv_seek_wake;
    std::condition_variable cv_game_end_req;
    std::condition_variable cv_gameover;
    volatile bool cv_game_over_flag = false;

    volatile bool cv_new_table_req_flag = false;

    std::vector<QueuePosition> queue = {};
    std::vector<int> table_numbers = {};

    int table_number = 0;
    std::set<int> companions = {};
    volatile int end_ready = 0;
    int chosen_game = 0;
    unsigned int players_acknowledged[PLAYER_NUM] = {};
};

void StateIdleLogic(Context *ctx);
void StatePlayLogic(Context *ctx);
void StateSeekLogic(Context *ctx);

void ProcessSignal(MPIMessage &d, Context* ctx);
void ProcessSIG_TABLE_REQ(MPIMessage &d, Context* ctx);
void ProcessSIG_SIG_TABLE_ACK(MPIMessage &d, Context* ctx);
void ProcessSIG_TABLE(MPIMessage &d, Context* ctx);
void ProcessSIG_END_REQ(MPIMessage &d, Context* ctx);
void ProcessSIG_GAME_END(MPIMessage &d, Context* ctx);
