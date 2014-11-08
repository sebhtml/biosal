#!/bin/sh

# Installation of prerequisites for a C Makefile build

# install required Ubuntu packages

sudo apt-get update -qq

# This is to test the MPI build with zlib support. We also need rt which
# is already installed by default (I think)

sudo apt-get install -qq openmpi-bin libopenmpi-dev zlib1g-dev
