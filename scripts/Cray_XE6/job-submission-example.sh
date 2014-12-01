#!/bin/bash
#PBS -N spate-work-stealing-607-256x24-10
#PBS -A CI-DEB000002
#PBS -l walltime=1:00:00
#PBS -l mppwidth=6144

cd $PBS_O_WORKDIR

# Cray XE6: http://www.training.prace-ri.eu/uploads/tx_pracetmo/comms.pdf
# Cray XC: http://www.training.prace-ri.eu/uploads/tx_pracetmo/MPI-environment-variables.pdf

# turn on async progress
# this is similar to PAMID_THREAD_MULTIPLE=1 for MPI on Blue Gene/Q (PAMID)
export MPICH_NEMESIS_ASYNC_PROGRESS=1
export MPICH_MAX_THREAD_SAFETY=multiple

# -r 1 tells aprun to use 1 core for specialization
# The nemesis engine in MPI will then be able to use that.
aprun -n 256 -N 1 -d 23 -r 1 \
              spate -threads-per-node 23 -print-thorium-data \
              -k 43 Iowa_Continuous_Corn/*.fastq -o spate-work-stealing-607-256x24-10 > spate-work-stealing-607-256x24-10.stdout

