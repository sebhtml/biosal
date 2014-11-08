#!/bin/bash

source tests/test_library.sh

function run_example()
{
    local example
    local result
    local load_lines
    local time_value

    example=$1

    echo -n "ExampleTest $example"

    (
    time make $example
    ) &> $example.log

    time_value=$(tail -n3 $example.log|head -n1|awk '{print $2}')
    load_lines=$(grep "COMPUTATION LOAD" $example.log | grep node | wc -l)
    result="FAILED"

    if test $load_lines -gt 0
    then
        result="PASSED"
    fi
    echo " result: $result time: $time_value (see $example.log)"
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

    biosal_shell_summarize_test_result "ExampleTestSuite" $passed $failed
}

main
