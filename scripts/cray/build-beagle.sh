#!/bin/bash

#module purge

# Cray environment
#module load PrgEnv-cray/4.2.24

# Cray version of MPICH
#module load cray-mpich/6.0.2

# Cray compiler
#module load cce/8.1.4

# reload the module
module swap PrgEnv-cray/4.2.24 PrgEnv-cray/4.2.24

make clean
make CC=cc -j 4
