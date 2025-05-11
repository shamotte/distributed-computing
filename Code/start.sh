#!/bin/bash

mpic++ -std=c++17 *.cpp -g -o main && mpirun -np $1 main; rm main