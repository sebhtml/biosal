#!/bin/bash

center=nersc
system=hopper
nodes=200
threads=24
walltime=03:00:00

build_script=scripts/Cray_XE6/build-script-hopper.sh
app_path=applications/spate_metagenome_assembler/spate
job_template=tests/Hopper_Cray_XE6/Template.spate.pbs

root=/project/projectdirs/m1523/Jobs-Hopper
dataset=/scratch/scratchdirs/boisvert/Iowa_Native_Prairie_Soil

submit_command=qsub

source tests/submit-actor-job-to-scheduler.sh


