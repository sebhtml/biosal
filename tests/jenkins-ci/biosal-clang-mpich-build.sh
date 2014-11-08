#!/bin/bash

mpicc -v

make clean
make
make unit-tests
#make examples
