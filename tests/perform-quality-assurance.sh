#!/bin/bash

function run_tests()
{
    echo "The whole quality assurance should take around 17 minutes"

    echo ""
    echo "Building products, please wait."
    scripts/build/build.sh &> /dev/null

    echo ""
    echo "Unit tests (make tests)"
    make -s tests | tail -n2

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
