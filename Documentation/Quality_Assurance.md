Before each commit, ideally these tests are run with 'make qa'

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

# Fields to fill for tests on big machines

```
**JobName**
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
**Checksum**
**GoodComments**
**BadComments**
**NeutralComments**
```
