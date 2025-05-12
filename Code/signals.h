#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <sstream>

#include "config.h"
#include "functions.h"
#include "states.h"
#include "signals.h"

#include <mpi.h>

extern int RANK, SIZE;
extern unsigned int global_lamport;
extern MPI_Datatype my_data;

void Broadcast_SIG_TABLE_REQ(int priority, int vote);

void Send_SIG_SIG_TABLE_ACK(int dest);

void Send_SIG_TABLE(int dest, std::set<int> companions, int table_number, int chosen_game);

void Send_SIG_END_REQ(int dest);

void Broadcast_SIG_GAME_END(std::set<int> players, int table_number);