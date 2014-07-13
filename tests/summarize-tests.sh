#!/bin/bash

function main()
{
    local file
    local passed_tests
    local failed_tests
    local total_tests

    file=$1

    passed_tests=$(grep ^PASSED $file | sed 's=/= =g' | awk '{print $2}' | awk '{ sum += $1} END {print sum}')
    failed_tests=$(grep ^FAILED $file | sed 's=/= =g' | awk '{print $2}' | awk '{ sum += $1} END {print sum}')
    total_tests=$(grep ^FAILED $file | sed 's=/= =g' | awk '{print $3}' | awk '{ sum += $1} END {print sum}')

    echo ""
    #echo "****"
    #echo "Summary:"
    echo "PASSED: $passed_tests/$total_tests"
    echo "FAILED: $failed_tests/$total_tests"

    #echo ""

    #if test $failed_tests -eq 0
    #then
        #echo "Result: perfect score (no action is required)"
    #else
        #echo "Result: some tests failed, see above"
    #fi
}

main $1
