#!/bin/bash

# crontab:
## everyday at 03:01 UTC-5 (8:01 UTC)
#1 8 * * * /space/boisvert/automated-tests/biosal/tests/run-tests-and-push-results.sh /space/boisvert/automated-tests

function main()
{
    local bucket
    local directory
    local object_prefix
    local log
    local test_name
    local repository
    local topic
    local bucket_name
    local address
    local archive
    local branch
    local detected_failures
    local subject
    local state

    branch="master"

    directory=$1
    mkdir -p $directory
    cd $directory

    test_name=$(date +%Y-%m-%d-%H:%M:%S)

    bucket_name="biosal"
    bucket="s3://$bucket_name"
    object_prefix="quality-assurance-department"

    log=$test_name".log"
    archive=$test_name".tar.xz"

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
    (
    git clone $repository
    git checkout $branch
    ) &>git-clone-checkout.log

    cd biosal
    echo "Branch: $branch"
    echo "Commit: $(git log | head -n1 | awk '{print $2}')"

    echo ""
    echo "Running quality assurance program"

    time make qa

    echo ""
    echo "Completed program"

    echo "Uploading $log and $archive to the cloud at $bucket/$object_prefix"

    ) &> $log

    tar -c *.log */*.log | xz -9 > $archive

    (
    aws s3 cp $log "$bucket/$object_prefix/$log"
    aws s3 cp $archive "$bucket/$object_prefix/$archive"
    )&> s3.log

    topic="arn:aws:sns:us-east-1:584851907886:biosal-tests"
    address=" http://$bucket_name.s3.amazonaws.com/$object_prefix/$log"

    detected_failures=$(cat $log | grep FAILED | wc -l)

    if test $detected_failures -gt 0
    then
        state="FAILED"
    else
        state="PASSED"
    fi

    subject="[biosal_bot][state=$state] biosal quality assurance report"

    aws sns publish --topic-arn $topic --subject "$subject" --message "A quality assurance report (state=$state) is available at $address"
}

main $1
