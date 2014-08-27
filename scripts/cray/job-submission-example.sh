#!/bin/bash
#PBS -N spate-work-stealing-607-256x24-10
#PBS -A CI-DEB000002
#PBS -l walltime=1:00:00
#PBS -l mppwidth=6144

cd $PBS_O_WORKDIR

# http://www.training.prace-ri.eu/uploads/tx_pracetmo/MPI-environment-variables.pdf
export MPICH_GNI_MAX_EAGER_MSG_SIZE=16384
export MPICH_GNI_NUM_BUFS=128

# turn on async progress
# this is similar to PAMID_THREAD_MULTIPLE=1 for MPI on Blue Gene/Q (PAMID)
export MPICH_NEMESIS_ASYNC_PROGRESS=1
export MPICH_MAX_THREAD_SAFETY=multiple
export MPICH_GNI_USE_UNASSIGNED_CPUS=enabled

# disable dynamic connections because we are using all-to-all anyway
export MPICH_GNI_DYNAMIC_CONN=disabled

aprun -n 256 -N 1 -d 24 \
              spate -threads-per-node 24 -print-load \
              -k 43 Iowa_Continuous_Corn/*.fastq -o spate-work-stealing-607-256x24-10 > spate-work-stealing-607-256x24-10.stdout

