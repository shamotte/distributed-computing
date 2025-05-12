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

#include <mpi.h>

extern int RANK, SIZE;
extern unsigned int global_lamport;
extern MPI_Datatype my_data;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}


void Broadcast_SIG_TABLE_REQ(int priority, int vote)
{
    global_lamport++;
    Datatype d;
    d.priority = priority;
    d.vote = vote;
    d.pid = RANK;
    d.type = SIG_TABLE_REQ;
    d.lamport = global_lamport;

    for (int i = 0; i < SIZE; i++)
    {

        MPI_Send(&d, 1, my_data, i, 0, MPI_COMM_WORLD);
    }
}

void Send_SIG_SIG_TABLE_ACK(int dest)
{
    global_lamport++;
}

void Send_SIG_TABLE(int dest, std::set<int> companions, int table_number, int chosen_game)
{
    global_lamport++;

    Datatype d;
    d.type = SIG_TABLE;

    std::copy(companions.begin(), companions.end(), d.players);

    d.table_number = table_number;
    d.chosen_game = chosen_game;

    d.lamport = global_lamport;
    d.pid = RANK;

    MPI_Send(&d, 1, my_data, dest, 0, MPI_COMM_WORLD);
}

void Send_SIG_END_REQ()
{
    global_lamport++;

} // zgłoszenie gotowości do zakończenia gry
void Broadcast_SIG_GAME_END(std::set<int> players, int table_number)
{
    global_lamport++;
}