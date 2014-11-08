#!/bin/bash

function run_tests_private()
{

    echo ""
    echo "Building products, please wait."
    (
    make clean
    make -j
    ) &> build.log
    echo "see build.log"

    echo ""
    echo "Unit tests (make unit-tests)"
    make -s unit-tests

    echo ""
    echo "Examples (make example-tests)"
    make -s example-tests

    echo ""
    echo "Applications (make application-tests)"
    make -s application-tests
}

function run_tests()
{
    local failed
    local result

    result="PASSED"
    run_tests_private | tee private_qa_log.log

    failed=$(cat private_qa_log.log | grep FAILED | wc -l)

    if test $failed -gt 0
    then
        result="FAILED"
    fi

    echo ""
    echo "Global result: $result"
}

function main()
{
    run_tests | tee qa.log
    echo ""
    echo "see qa.log"
}

main
