#!/bin/bash

center=nersc
system=edison
nodes=128
threads=24
walltime=00:30:00

build_script=scripts/Cray_XC30/build-spate-intel.sh
app_path=performance/latency_probe/latency_probe
job_template=tests/Edison_Cray_XC30/Template.latency_probe.pbs

root=/project/projectdirs/m1523/Jobs-Edison
dataset=/dev/null

submit_command=qsub

source tests/submit-actor-job-to-scheduler.sh

