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
#include "state.h"
#include "signalSender.h"

#include <mpi.h>

extern int RANK, SIZE;
extern unsigned int global_lamport;
extern MPI_Datatype my_data;

void Broadcast_SIG_TABLE_REQ(int priority, int vote)
{
    global_lamport++;

    MPIMessage d;
    d.type = SIG_TABLE_REQ;
    d.lamport = global_lamport;
    d.pid = RANK;

    d.priority = priority;
    d.vote = vote;

    for (int i = 0; i < SIZE; i++)
    {

        MPI_Send(&d, 1, my_data, i, 0, MPI_COMM_WORLD);
    }
}

void Send_SIG_SIG_TABLE_ACK(int dest)
{
    global_lamport++;

    MPIMessage d;
    d.type = SIG_TABLE_ACK;
    d.lamport = global_lamport;
    d.pid = RANK;

    MPI_Send(&d, 1, my_data, dest, 0, MPI_COMM_WORLD);
}

void Send_SIG_TABLE(int dest, std::set<int> companions, int table_number, int chosen_game)
{
    global_lamport++;

    MPIMessage d;
    d.type = SIG_TABLE;
    d.lamport = global_lamport;
    d.pid = RANK;

    std::copy(companions.begin(), companions.end(), d.players);

    d.table_number = table_number;
    d.chosen_game = chosen_game;

    MPI_Send(&d, 1, my_data, dest, 0, MPI_COMM_WORLD);
}

void Send_SIG_END_REQ(int dest)
{
    global_lamport++;

    MPIMessage d;
    d.type = SIG_END_REQ;
    d.lamport = global_lamport;
    d.pid = RANK;

    MPI_Send(&d, 1, my_data, dest, 0, MPI_COMM_WORLD);

} // zgłoszenie gotowości do zakończenia gry

void Broadcast_SIG_GAME_END(std::set<int> players, int table_number)
{
    global_lamport++;

    MPIMessage d;
    d.type = SIG_GAME_END;
    d.lamport = global_lamport;
    d.pid = RANK;

    std::copy(players.begin(), players.end(), d.players);

    d.table_number = table_number;

    for (int i = 0; i < SIZE; i++)
    {

        MPI_Send(&d, 1, my_data, i, 0, MPI_COMM_WORLD);
        coutcolor("wysylam SIG_GAME_END do ", i);
    }
}
