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

#include "config.h"
#include "functions.h"

enum State
{
    STATE_IDLE,
    STATE_SEEK,
    STATE_PLAY
};

class BaseState; // deklarujemy klasę base state

class Context
{
public:
    BaseState *current_state;
    std::mutex state_mutex;
    int priority = 0;

    std::vector<int> queue = {};
    std::vector<int> table_numbers = {};

    int table_number = 0;
    std::set<int> companions = {};
    int end_ready = 0;
    int chosen_game = 0;
    bool players_acknowledged[PLAYER_NUM] = {};

    std::thread logic_thread;
};
class BaseState
{
public:
    Context *ctx;

    virtual void Logic();
    virtual void ProcessSignal();

    void changeState(State new_state);
};

class StateIdle : BaseState
{
};

class StateSeek : BaseState
{
};

class StatePlay : BaseState
{
};
