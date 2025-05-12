#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <sstream>

enum MessageType
{
    SIG_TABLE_REQ = 0,
    SIG_TABLE_ACK,
    SIG_TABLE,
    SIG_END_REQ,
    SIG_GAME_END
};
const std::string MessageNames[] = {
    "SIG_TABLE_REQ",
    "SIG_TABLE_ACK",
    "SIG_TABLE",
    "SIG_END_REQ",
    "SIG_GAME_END"
};

struct Datatype
{
    unsigned int lamport;
    MessageType type;
    unsigned int pid;

    int players[SEAT_COUNT] = {};

    union
    {
        int table_number;
        int priority;
    };

    union
    {
        int vote;
        int chosen_game;
    };
};

template <typename... Args>
void coutcolor(Args &&...args);

void check_thread_support(int provided);

void Broadcast_SIG_TABLE_REQ(int priority, int vote);

void Send_SIG_SIG_TABLE_ACK(int dest);

void Send_SIG_TABLE(int dest, std::set<int> companions, int table_number, int chosen_game);

void Send_SIG_END_REQ(int dest);

void Broadcast_SIG_GAME_END(std::set<int> players, int table_number);
