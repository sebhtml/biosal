#!/bin/bash

target="unit-tests"

echo "JOB_NAME= $JOB_NAME"
echo "build= $build"
echo "compiler= $compiler"

# the job-system-tests Jenkins projects runs all tests
if test $JOB_NAME = "big-system-tests"
then
    module load mpich/3.1.1-1

    target="all-tests"
fi

if $compiler = "gcc"
then
    echo "" > /dev/null
elif $compiler = "clang"
then
    echo "" > /dev/null
fi

cc -v
mpicc -v

make clean

options=""

if $build = "default"
then
    echo "" > /dev/null
elif $build = "debug"
then
    options="$options CONFIG_DEBUG=y"
fi

# build executables
make $options

# run tests
make $target
