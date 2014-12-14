#!/bin/bash

center=alcf
system=cetus
nodes=512
threads=16
walltime=01:00:00

build_script=scripts/IBM_Blue_Gene_Q/build-spate-xl.sh
app_path=applications/spate_metagenome_assembler/spate
job_template=tests/Mira_IBM_Blue_Gene_Q/Template.spate.sh

root=/projects/CompBIO/Projects/automated-tests-cetus
dataset=/gpfs/mira-fs1/projects/CompBIO/Datasets/JGI/Great_Prairie_Soil_Metagenome_Grand_Challenge/Datasets/Iowa_Native_Prairie_Soil

submit_command=bash

source tests/submit-actor-job-to-scheduler.sh

