#!/bin/bash

function run_tests()
{

    echo ""
    echo "Building products, please wait."
    scripts/build/build.sh &> build.log
    echo "see build.log"

    echo ""
    echo "Unit tests (make tests)"
    make -s tests > unit-tests.log
    tail -n 2 unit-tests.log
    echo "see unit-tests.log"

    echo ""
    echo "Real use cases (tests/run-integration-tests.sh)"
    tests/run-integration-tests.sh

    echo ""
    echo "Examples (make examples)"
    make -s examples
}

function main()
{
    run_tests | tee qa.log
    echo ""
    echo "see qa.log"
}

main
