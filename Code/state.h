#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
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

class BaseState; // deklarujemy klasę base state

class Context
{
public:
    BaseState *current_state;
    State next_state;
    std::map<State, BaseState *> States;
    std::mutex state_mutex;
    int priority = 0;
    std::condition_variable cv_seek;
    std::condition_variable cv_seek_wake;
    std::condition_variable cv_game_end_req;
    std::condition_variable cv_gameover;
    volatile bool cv_game_over_flag = false;

    std::vector<QueuePosition> queue = {};
    std::vector<int> table_numbers = {};

    std::mutex seek_mutex;
    std::mutex seek_wake_mutex;

    std::mutex play_wait_mutex;
    std::mutex play_end_mutex;

    int table_number = 0;
    std::set<int> companions = {};
    volatile int end_ready = 0;
    int chosen_game = 0;
    unsigned int players_acknowledged[PLAYER_NUM] = {};

    std::thread logic_thread;

    BaseState *GetNextState();
};
class BaseState
{
public:
    Context *ctx;

    virtual void Logic();
    virtual void ProcessSignal(MPIMessage &d);

    virtual void ProcessSIG_TABLE_REQ(MPIMessage &d);
    virtual void ProcessSIG_SIG_TABLE_ACK(MPIMessage &d);
    virtual void ProcessSIG_TABLE(MPIMessage &d);
    virtual void ProcessSIG_END_REQ(MPIMessage &d);
    virtual void ProcessSIG_GAME_END(MPIMessage &d);
};

class StateIdle : public BaseState
{
public:
    StateIdle(Context *ctx);

    void Logic();
};

class StateSeek : public BaseState
{
public:
    StateSeek(Context *ctx);

    void Logic();
};

class StatePlay : public BaseState
{
public:
    StatePlay(Context *ctx);
    void Logic();
};
