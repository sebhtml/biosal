#!/usr/bin/env python

import os
import sys
import subprocess
from cStringIO import StringIO
from subprocess import Popen, PIPE


# \see http://bugs.python.org/issue7471
#import gzip

def open_file2(file):

    if file.find(".gz") >= 0:
        #return os.popen("cat " + file + " | zcat", "r", 8388608)
        process = subprocess.Popen(["zcat", file], stdout = subprocess.PIPE)
        descriptor = StringIO(process.communicate()[0])
        return descriptor

# \see http://spyced.blogspot.com/2006/12/wow-gzip-module-kinda-sucks.html
def open_file(fname):
    f = Popen(['zcat', fname], stdout=PIPE)
    for line in f.stdout:
        yield line

if len(sys.argv) == 1:
    print("A dataset directory with sequence files (.fastq.gz) is required")
    print("Example: ./summarize_dataset.py Iowa_Continuous_Corn_Soil")
    sys.exit()

dataset = sys.argv[1]

files = []
for file in os.listdir(dataset):
    files.append(file)

files.sort()

dataset_files = 0
dataset_reads = 0
dataset_bases = 0

debug = 0

print("Dataset: " + dataset)
print("")

for file in files:

    # check if the file contains paired reads
    paired_status = "No"

    #descriptor = gzip.open(dataset + "/" + file)


    i = 0
    line0 = ""
    line4 = ""

    for line in open_file(dataset + "/" + file):
        if i == 0:
            line0 = line.split()[0]
        elif i == 4:
            line4 = line.split()[0]

        if i == 4:
            break

        i += 1


    progress_period = 1000000
    difference = 0

    i = 0

    while i < len(line0):
        if line0[i] != line4[i]:
            difference += 1

        i += 1

    if difference == 1:
        paired_status = "Yes"

    distribution = {}

    i = 0

    reads = 0
    bases = 0

    for line in open_file(dataset + "/" + file):
        if i % 4 == 1:
            sequence = line.strip()
            sequence_bases = len(sequence)

            reads += 1
            bases += sequence_bases

            if reads % progress_period == 0 and debug:
                print("PROGRESS " + str(reads))
                sys.stdout.flush()

            if sequence_bases not in distribution:
                distribution[sequence_bases] = 0
            distribution[sequence_bases] += 1

        if i > 10000 and debug:
            break

        i += 1


    keys = distribution.keys()

    keys.sort()

    
    # Display a report
    print(file)
    print("  Paired: " + paired_status)
    width = 16

    print("  " + "ReadLength".ljust(width) + "ReadCount".ljust(width) + "Bases")

    for read_length in keys:
        local_reads = distribution[read_length]
        print("  " + str(read_length).ljust(width) + str(local_reads).ljust(width) + str(local_reads * read_length))

    print("  " + "*".ljust(width) + str(reads).ljust(width) + str(bases))

    dataset_files += 1
    dataset_reads += reads
    dataset_bases += bases

    print("")
    sys.stdout.flush()

print("")
print("Summary")
print("  Dataset: " + dataset)
print("  FileCount: " + str(dataset_files))
print("  ReadCount: " + str(dataset_reads))
print("  BaseCount: " + str(dataset_bases))


