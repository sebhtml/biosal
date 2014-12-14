#!/bin/bash

center=nersc
system=edison
nodes=128
threads=24
walltime=03:00:00

build_script=scripts/Cray_XC30/build-spate-intel.sh
app_path=applications/spate_metagenome_assembler/spate
job_template=tests/Edison_Cray_XC30/Template.spate.pbs

root=/project/projectdirs/m1523/Jobs-Edison
dataset=/project/projectdirs/m1523/Data/Iowa_Native_Prairie_Soil

submit_command=qsub

source tests/submit-actor-job-to-scheduler.sh

