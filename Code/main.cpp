
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

int RANK, SIZE;
template <typename... Args>

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

void SignalProcesingLoop(Context *ctx)
{
    while (true)
    {
        Datatype d;
        MPI_Status status;
        MPI_Recv(&d, 1, my_data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        coutcolor("otrzymano od ", d.priority, "od", status.MPI_SOURCE);
        // obieranie sygbnałów

        std::unique_lock(ctx->state_mutex);

        ctx->current_state->ProcessState(d);
    }
}

void ContinousLogic(Context *ctx)
{
    std::thread logic_thread;
    while (true)
    {
        logic_thread = std::thread(&BaseState::Logic, ctx->current_state);

        logic_thread.join();

        std::unique_lock(ctx->state_mutex);
        ctx->current_state = ctx->GetNextState();
    }
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

    Context *ctx = new Context();

    std::map<State, BaseState *> &states = ctx->States;

    states[STATE_IDLE] = new StateIdle();
    states[STATE_SEEK] = new StateSeek();
    states[STATE_PLAY] = new StatePlay();

    ctx->current_state = states[STATE_IDLE];

    std::thread signalProcessor(SignalProcesingLoop, ctx);
    std::thread logicThread(ContinousLogic, ctx);

    logicThread.join();
    signalProcessor.join();

#pragma region finalization
    std::cout << "rank " << RANK << "\n";
    MPI_Type_free(&my_data);
    MPI_Finalize();
    return 0;

#pragma endregion
}
