# Source code

For style, see [CodingStyle.md](CodingStyle.md).

# Continuous integration

We use the Integration Manager Workflow (http://git-scm.com/about/distributed) to
integrate changes in the master tree.

The project has a Jenkins deployment.
The build machines are in Magellan/OpenStack at Argonne and at Oak Ridge (bare metal).
The DNS name currently ends with boisvert.info, pending a DNS A entry at MCS.

- The dashboard is available at http://jenkins.biosal.anl.boisvert.info:8080/

- Every 5 minutes, the system checks for change and create builds with Clang and with GCC.

- Every night at 3 AM, a crontab runs all tests and sends a notification with Amazon SNS.

- Everynight at 5 AM, Jenkins runs all the tests.

# Before committing

Before each commit, ideally these tests are run with 'make qa'.

./script/build.sh

./script/check.sh

make tests

make examples

scripts/development/mini.sh
scripts/development/medium.sh
scripts/development/medium-2.sh
scripts/development/run-argonnite-1-28.sh


# S3 bucket with public stuff

- s3://biosal
- HTML listing: http://biosal.s3.amazonaws.com/index.html
- XML listing: http://biosal.s3.amazonaws.com/
- XML listing: https://s3.amazonaws.com/biosal/

# Data required for the tests

- Sizes: http://biosal.s3.amazonaws.com/testing-data/ls-lh.txt
- Checksums: http://biosal.s3.amazonaws.com/testing-data/sha1sum.txt
- Compressed with: http://biosal.s3.amazonaws.com/testing-data/xz.txt
- http://biosal.s3.amazonaws.com/testing-data/mini.fastq.xz
- http://biosal.s3.amazonaws.com/testing-data/medium-2.fastq.xz
- http://biosal.s3.amazonaws.com/testing-data/medium.fastq.xz
- http://biosal.s3.amazonaws.com/testing-data/GPIC.1424-1.1371.fastq.xz

# Tested platforms

| Platform | Compiler | MPI library | Scheduler | Compute operating system |
| --- | --- | --- | --- | --- |
| IBM Blue Gene/Q | IBM XL C/C++ for Blue Gene, V12.1 Version: 12.01.0000.0008 | IBM MPI | Argonne Cobalt | IBM CNK (Compute Node Kernel) |
| Cray XE6 | Cray C : Version 8.1.4 | Cray MPI | moab client 6.1.1 | Cray CNL (Compute Node Linux) |
| Cray XE6 | gcc version 4.8.1 20130531 (Cray Inc.) (GCC) | Cray MPI | moab client 6.1.1 | Cray CNL (Compute Node Linux) |
| Linux server | gcc (GCC) 4.4.7 20120313 (Red Hat 4.4.7-4) | MPICH 3.1.1 | - | CentOS release 6.5 (Final) |

- for Beagle (Cray XE6): tests/Beagle_Cray_XE6/
- for Cetus (IBM Blue Gene/Q): tests/Cetus_IBM_Blue_Gene_Q/

# Fields to fill for QA reports

```
**JobName**
**Goal**
**Machine**
**AllocationStatus**
**Path**
**Commit**
**Toolchain**
**Script**
**Submission**
**MachineUtilization**
**ComputationLoad**
**RunningTime**
**MemoryUtilization**
**Checksum**
**GoodComments**
**BadComments**
**NeutralComments**
```
