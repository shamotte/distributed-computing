#!/bin/bash

rm main
mpic++ -std=c++17 *.cpp -g -o main
#mpirun -np $1 main
