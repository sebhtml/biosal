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

if test $compiler = "gcc"
then
    echo "" > /dev/null
elif test $compiler = "clang"
then
    echo "" > /dev/null
fi

echo "cc -v"
cc -v

echo "mpicc -v"
mpicc -v

echo "mpiexec -v"
mpiexec -v

make clean

options=""

if test $build = "default"
then
    echo "" > /dev/null
elif test $build = "debug"
then
    options="$options CONFIG_DEBUG=y"
fi

# build executables
make $options

# run tests
make $target
