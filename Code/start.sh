#!/bin/bash

mpic++ *.cpp -g -o main && mpirun -np $1 main; rm main