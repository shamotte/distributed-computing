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
    "SIG_GAME_END"};

struct MPIMessage
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

void check_thread_support(int provided);

extern int RANK, SIZE;
extern unsigned int global_lamport;
extern unsigned int games_played;
extern std::string global_state_name;

const std::string PlayerNames[] = {
    "Eustachy",
    "Telimena",
    "Mieczysław",
    "Eugenia",
    "Juliusz",
    "Eleonora",
    "Edek",
    "Daniel J. D'Arby",
    "Michał",
    "Hiacynta",
    "Jack Black",
    "Yumeko",
    "Freddy Fazbear",
    "Gerwazy",
    "Laweta",
    "Leonidas",
};

template <typename... Args>
void coutcolor(Args &&...args)
{
    std::ostringstream oss;
    (oss << ... << args); // składnia fold expression (C++17)
    std::cout
        << "\033[0;" << (31 + RANK % 7) << "m"
        << "["
        << global_lamport
        << "\t] "
        << "("
        << RANK
        << ": "
        << games_played
        << ") "
        << PlayerNames[RANK % (sizeof(PlayerNames) / sizeof(*PlayerNames))]
        << "<" << global_state_name << ">"
        << "\t"
        << oss.str()
        << "\033[0m\n";
}
