
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
#include "state.h"
#include "signalSender.h"

MPI_Datatype my_data;
unsigned int global_lamport;
int RANK, SIZE;
unsigned int games_played;

void SignalProcesingLoop(Context *ctx)
{
    while (true)
    {
        MPIMessage d;
        MPI_Status status;
        MPI_Recv(&d, 1, my_data, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        global_lamport = std::max(global_lamport, d.lamport) + 1;
        coutcolor("otrzymano  ", d.priority, "od ", PlayerNames[d.pid], " o typie ", MessageNames[d.type], " i timestampie :", d.lamport);

        std::unique_lock(ctx->state_mutex);

        ctx->current_state->ProcessSignal(d);
    }
}

void ContinousLogic(Context *ctx)
{
    std::thread logic_thread;
    while (true)
    {
        logic_thread = std::thread(&BaseState::Logic, ctx->current_state);

        logic_thread.join();
        coutcolor("zakończyłem logic idę dalej");

        std::unique_lock(ctx->state_mutex);
        ctx->current_state = ctx->States[ctx->next_state];
    }
}

int main(int argc, char **argv)
{
#pragma region initialization
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int NITEMS = 6;

    int blockSizes[] = {1, 1, 1, SEAT_COUNT, 1, 1};
    MPI_Datatype types[] = {MPI_UNSIGNED, MPI_INT, MPI_UNSIGNED, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offset[NITEMS] = {
        offsetof(MPIMessage, lamport),
        offsetof(MPIMessage, type),
        offsetof(MPIMessage, pid),
        offsetof(MPIMessage, players),
        offsetof(MPIMessage, table_number),
        offsetof(MPIMessage, vote)};

    MPI_Type_create_struct(NITEMS, blockSizes, offset, types, &my_data);
    MPI_Type_commit(&my_data);

    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

#pragma endregion

    srand(time(NULL));

    Context *ctx = new Context();

    std::map<State, BaseState *> &states = ctx->States;

    states[STATE_IDLE] = new StateIdle(ctx);
    states[STATE_SEEK] = new StateSeek(ctx);
    states[STATE_PLAY] = new StatePlay(ctx);

    ctx->current_state = states[STATE_IDLE];

    ctx->table_numbers;
    for (int i = 0; i < TABLE_NUM; i++)
    {
        ctx->table_numbers.push_back(i);
    }

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
