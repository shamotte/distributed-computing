#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <sstream>
#include <mpi.h>

#include "config.h"
#include "functions.h"
#include "states.h"

MPI_Datatype my_data;

struct Datatype
{
    unsigned int lamport;
    MessageType type;

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

int RANK, SIZE;
template <typename... Args>
void coutcolor(Args &&...args)
{
    std::ostringstream oss;
    (oss << ... << args); // składnia fold expression (C++17)
    std::cout << "\033[0;" << (31 + RANK % 7) << "m"
              << oss.str()
              << "\033[0m\n";
}

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
    Datatype d;
    d.priority = priority;
    d.vote = vote;

    for (int i = 0; i < SIZE; i++)
    {
        MPI_Send(&d, 1, my_data, i, 0, MPI_COMM_WORLD);
    }
}

void Send_SIG_SIG_TABLE_ACK()
{
}

void Send_SIG_TABLE(std::set<int> companions, int table_number, int chosen_game)
{
}

void Send_SIG_END_REQ()
{

} // zgłoszenie gotowości do zakończenia gry
void Broadcast_SIG_GAME_END(std::set<int> players, int table_number)
{
}

int main(int argc, char **argv)
{
#pragma region initialization
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int NITEMS = 5;

    int blockSizes[] = {1, 1, SEAT_COUNT, 1, 1};
    MPI_Datatype types[] = {MPI_UNSIGNED, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offset[NITEMS] = {
        offsetof(Datatype, lamport),
        offsetof(Datatype, type),
        offsetof(Datatype, players),
        offsetof(Datatype, table_number),
        offsetof(Datatype, vote)};

    MPI_Type_create_struct(NITEMS, blockSizes, offset, types, &my_data);
    MPI_Type_commit(&my_data);

    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    if (RANK == 0)
    {
        Broadcast_SIG_TABLE_REQ(20, 1);
    }
    else
    {
        Datatype d;
        MPI_Status status;
        MPI_Recv(&d, 1, my_data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        coutcolor("otrzymano od ", status.MPI_SOURCE, "od", d.priority);
    }

#pragma endregion

    Context *ctx = Context();
    ctx->current_state = StateIdle();
    current_state.EnterState();
    while (true)
    {
        Datatype d;
        MPI_Status status;
        MPI_Recv(&d, 1, my_data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        coutcolor("otrzymano od ", d.priority, "od", tatus.MPI_SOURCE);
        // obieranie sygbnałów

        std::unique_lock(current_state.ctx->state_mutex);

        current_state.ProcessSignal();
    }

#pragma region finalization
    std::cout << "rank " << RANK << "\n";
    MPI_Type_free(&my_data);
    MPI_Finalize();
    return 0;

#pragma endregion
}
