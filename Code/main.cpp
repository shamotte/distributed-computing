#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <mpi.h>

#include "config.h"
MPI_Datatype my_data;
enum MessageType
{
    SIG_TABLE_REQ = 0,
    SIG_TABLE_ACK,
    SIG_TABLE,
    SIG_END_REQ,
    SIG_GAME_END
};

struct Datatype
{
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

    MPI_Bcast(&d, 1, my_data, RANK, MPI_COMM_WORLD)
}

void Send_SIG_SIG_TABLE_ACK()
{
}

void Send_SIG_TABLE(set<int> companions, int table_number, int chosen_game)
{ // danie znać innym, że należą do stołu
}

void Send_SIG_END_REQ()
{

} // zgłoszenie gotowości do zakończenia gry
void Broadcast_SIG_GAME_END(set<int> players, int table_number)
{
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int NITEMS = 4;

    int blockSizes = {1, SEAT_COUNT, 1, 1};
    MPI_Datatype types = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offset[NITMES];

    offset[0] = offsetof(Datatype, type);
    offset[1] = offsetof(Datatype, players);
    offset[2] = offsetof(Datatype, value1);
    offset[3] = offsetof(Datatype, value2);

    MPI_Type_create_struct(NITEMS, blockSizes, offset, types, &my_data);
    MPI_Type_commit(&my_data);

    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    if (SIZE == 0)
    {
        Broadcast_SIG_TABLE_REQ(0, 1);
    }
    else
    {
        Datatype d;
        MPI_Status status;
        MPI_Recv(&d, 1, my_data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        std::cout << "odebrano " << d.priority << " od " << status.MPI_SOURCE << "\n";
    }

    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
    return 0;
}
