#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <set>
#include <sstream>

enum MessageType;

struct Datatype;

template <typename... Args>
void coutcolor(Args &&...args);

void check_thread_support(int provided);

void Broadcast_SIG_TABLE_REQ(int priority, int vote);

void Send_SIG_SIG_TABLE_ACK();

void Send_SIG_TABLE(std::set<int> companions, int table_number, int chosen_game);

void Send_SIG_END_REQ();

void Broadcast_SIG_GAME_END(std::set<int> players, int table_number);
