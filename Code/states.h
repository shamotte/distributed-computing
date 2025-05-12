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

#include "config.h"
#include "functions.h"

enum State
{
    STATE_IDLE,
    STATE_SEEK,
    STATE_PLAY,
    STATE_STAY
};

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
    int lamport = 0;

    std::vector<QueuePosition> queue = {};
    std::vector<int> table_numbers = {};

    int table_number = 0;
    std::set<int> companions = {};
    int end_ready = 0;
    int chosen_game = 0;
    bool players_acknowledged[PLAYER_NUM] = {};

    std::thread logic_thread;

    BaseState *GetNextState();
};
class BaseState
{
public:
    Context *ctx;

    virtual void Logic();
    virtual void ProcessState(Datatype &d);

    virtual void ProcessSIG_TABLE_REQ(Datatype &d);
    virtual void ProcessSIG_SIG_TABLE_ACK(Datatype &d);
    virtual void ProcessSIG_TABLE(Datatype &d);
    virtual void ProcessSIG_END_REQ(Datatype &d);
    virtual void ProcessSIG_GAME_END(Datatype &d);
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
