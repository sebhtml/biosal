#!/bin/bash

source tests/test_library.sh

function main()
{
    local file
    local passed_tests
    local failed_tests
    local failed_lines

    file=$1

    passed_tests=$(grep PASSED $file | sed 's=/= =g' | awk '{print $4}' | awk '{ sum += $1} END {print sum}')

    failed_tests=0

    failed_lines=$(cat $file | grep FAILED | wc -l)

    #echo "DEBUG failed_lines $failed_lines"

    if test $failed_lines -gt 0
    then
        failed_tests=$(grep FAILED $file | sed 's=/= =g' | awk '{print $6}' | awk '{ sum += $1} END {print sum}')
    fi

    biosal_shell_summarize_test_result "UnitTestSuite " $passed_tests $failed_tests
}

main $1
