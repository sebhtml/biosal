#!/bin/bash

echo "build= $build"
echo "compiler= $compiler"

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

if $build = "default"
then
    make
elif $build = "debug"
then
    make CONFIG_DEBUG=y
fi

make unit-tests
