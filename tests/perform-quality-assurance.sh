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
    echo "Unit tests (make tests)"

    make -s tests > unit-tests.log
    grep ^UnitTestSuite unit-tests.log
    echo "see unit-tests.log"

    echo ""
    echo "Examples (make examples)"
    make -s examples

    echo ""
    echo "Real use cases (tests/run-integration-tests.sh)"
    tests/run-integration-tests.sh
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
