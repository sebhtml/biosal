#!/bin/bash

center=alcf
system=mira
nodes=512
threads=16

build_script=scripts/IBM_Blue_Gene_Q/build-xl.sh
app_path=performance/latency_probe/latency_probe
job_template=tests/Mira_IBM_Blue_Gene_Q/Template.latency_probe.sh

root=/projects/CompBIO/Projects/automated-tests-mira
dataset=/gpfs/mira-fs1/projects/CompBIO/Datasets/JGI/Great_Prairie_Soil_Metagenome_Grand_Challenge/Datasets/Iowa_Native_Prairie_Soil

submit_command=bash

source tests/submit-actor-job-to-scheduler.sh

