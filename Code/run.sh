#!/bin/bash

mpirun -np "$1" --oversubscribe main
