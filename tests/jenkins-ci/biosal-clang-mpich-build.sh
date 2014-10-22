#!/bin/bash

mpicc -v

make clean
make
make tests
#make examples
