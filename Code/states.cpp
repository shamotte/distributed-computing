#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>

#include "config.h"

class BaseState
{
public:
    int priority = 0;

    std::vector<int> queue = {};
    std::vector<int> table_numbers = {};

    int table_number = 0;
    std::set<int> companions = {};
    int end_ready = 0;
    int chosen_game = 0;

    bool players_acknowledged[PLAYER_NUM] = {};

    virtual void Logic();
    virtual void ProcessSignal();

    void change_state()
    {
    }
};

class StateIdle : BaseState
{
public:
    void Logic()
    {
    }

    void ProcessSignal()
    {
    }
};

class StateSeek : BaseState
{
};

class StatePlay : BaseState
{
};
