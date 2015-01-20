#!/bin/bash

# http://dskernel.blogspot.ca/2014/08/the-public-datasets-from-doejgi-great.html?view=magazine
# http://beagle.ci.uchicago.edu/using-beagle/
# http://wiki.ci.uchicago.edu/Beagle/SystemSpecs#Traditional_Compute_Nodes_XE6_wi

center=ci
system=beagle2
nodes=36
threads=1152
allocation=CI-CCR000040
queue=batch
walltime=03:00:00

build_script=tests/Beagle_Cray_XE6/build.sh
app_path=applications/spate_metagenome_assembler/spate
job_template=tests/Beagle_Cray_XE6/Template.spate.pbs

root=/lustre/beagle2/CompBIO/automated-tests-beagle2
dataset=/lustre/beagle2/CompBIO/Datasets/Wisconsin_Restored_Prairie_Soil

submit_command=qsub

repository=git://github.com/sebhtml/biosal.git
branch=master

source tests/submit-actor-job-to-scheduler.sh
