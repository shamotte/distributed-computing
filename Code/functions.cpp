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

extern int RANK, SIZE;
extern unsigned int global_lamport;

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
		<< PlayerNames[RANK % (sizeof(PlayerNames)/sizeof(*PlayerNames))]
		<< "\t"
		<< oss.str()
		<< "\033[0m\n";
}
