#!/bin/bash

# crontab:
## everyday at 03:01 UTC-5 (8:01 UTC)
#1 8 * * * /space/boisvert/automated-tests/biosal/tests/run-tests-and-push-results.sh /space/boisvert/automated-tests

function main()
{
    local bucket
    local directory
    local object
    local log
    local test_name
    local repository

    directory=$1
    mkdir -p $directory
    cd $directory

    test_name=$(date +%Y-%m-%d-%H:%M:%S)

    bucket="s3://biosal"
    log=$test_name".txt"
    object="quality-assurance-department/"$log

    mkdir $test_name
    cd $test_name

    (
    echo "Beginning automated test"
    echo "test_name= $test_name"

    echo ""
    echo "Environment:"
    echo "Host: $(hostname)"
    echo "Directory: $(pwd)"
    echo "Date: $(date)"
    echo "Load: $(w | head -n 1)"

    echo ""
    echo "Hardware"
    head -n 2 /proc/meminfo
    grep "model name" /proc/cpuinfo | head -n 1
    echo "Processor core count: "$(grep "model name" /proc/cpuinfo | wc -l)

    echo ""
    echo "Versions"
    uname -a | head -n 1
    git --version | head -n 1
    mpicc --version | head -n 1
    mpiexec --version | head -n 2

    echo ""
    echo "Cloning blueprints"
    repository="https://github.com/sebhtml/biosal.git"
    echo "Repository: $repository"
    git clone $repository
    cd biosal
    echo "Commit: $(git log | head -n1 | awk '{print $2}')"

    echo ""
    echo "Running quality assurance program"

    time make qa

    echo ""
    echo "Completed program"

    echo "Uploading log to the cloud at $bucket/$object"
    ) &> $log

    # this is not an interactive session
    /usr/local/bin/aws s3 cp $log $bucket"/"$object &> s3.log
}

main $1
