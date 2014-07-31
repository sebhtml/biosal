#!/bin/bash

source tests/test_library.sh

function run_example()
{
    local example
    local result
    local load_lines

    example=$1

    echo -n "ExampleTest $example "

    (
    time make $example
    ) &> $example.log

    load_lines=$(grep "COMPUTATION_LOAD" $example.log | grep node | wc -l)
    result="FAILED"

    if test $load_lines -gt 0
    then
        result="PASSED"
    fi
    echo " result: $result (see $example.log)"
}

function main()
{
    local example
    local passed
    local failed
    local total

    for example in $(cat examples/example-tests.txt)
    do
        run_example $example
    done | tee examples.log

    passed=$(grep PASSED examples.log | wc -l)
    failed=$(grep FAILED examples.log | wc -l)

    bsal_shell_summarize_test_result "ExampleTestSuite" $passed $failed
}

main
