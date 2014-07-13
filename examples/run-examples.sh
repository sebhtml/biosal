#!/bin/bash

function run_example()
{
    local example
    local result
    local efficiency_lines

    example=$1

    echo -n "Running test case example:$example ..."

    (
    time make $example
    ) &> $example.log

    efficiency_lines=$(grep "efficiency" $example.log | grep node | wc -l)
    result="FAILED"

    if test $efficiency_lines -gt 0
    then
        result="PASSED"
    fi
    echo " result: $result, see $example.log"
}

function main()
{
    local example
    local passed
    local failed
    local total

    for example in mock mock1 ring reader not_found remote_spawn synchronize controller hello_world clone migration
    do
        run_example $example
    done | tee examples.log

    passed=$(grep PASSED examples.log | wc -l)
    failed=$(grep FAILED examples.log | wc -l)
    total=$(($passed + $failed))

    echo ""
    echo "PASSED: $passed/$total"
    echo "FAILED: $failed/$total"
}

main
