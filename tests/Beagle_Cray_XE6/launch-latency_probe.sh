#!/bin/bash

center=ci
system=beagle2
nodes=128
threads=32
walltime=00:30:00

build_script=tests/Beagle_Cray_XE6/build.sh
app_path=performance/latency_probe/latency_probe
job_template=tests/Beagle_Cray_XE6/Template.latency_probe.pbs

root=/lustre/beagle2/CompBIO/automated-tests-beagle2
dataset=/dev/null

submit_command=qsub

source tests/submit-actor-job-to-scheduler.sh

