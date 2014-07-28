#!/bin/bash

mpiexec -n 4 applications/argonnite -print-load -k 43 -threads-per-node 8 ~/dropbox/Iowa_Continuous_Corn/*
