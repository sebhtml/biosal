#!/bin/bash

source tests/test_library.sh

function main()
{
    local real_test
    local actual_sum
    local expected_sum
    local passed
    local failed
    local total
    local failed
    local wall_time

    #for real_test in $(cat tests/application-tests.txt | head -n 1)
    for real_test in $(cat tests/application-tests.txt)
    do
        rm -rf output &> /dev/null

        tests/run-application-test.sh $real_test > $real_test.log

        wall_time=$(tail -n 3 $real_test.log | grep real | awk '{print $2}')

        actual_sum="foo"

        if test -f "output/coverage_distribution.txt-canonical"
        then
            actual_sum=$(sha1sum output/coverage_distribution.txt-canonical | awk '{print $1}')
        fi
        expected_sum=$(grep "$real_test sha1sum" tests/checksums.txt | grep sha1sum | awk '{print $3}')

        result="FAILED"

        if test "$actual_sum" = "$expected_sum"
        then
            result="PASSED"
        fi

        echo "Test: $real_test Result: $result Time: $wall_time (see $real_test.log)"

    done | tee real.log

    passed=$(grep PASSED real.log | wc -l)
    failed=$(grep FAILED real.log | wc -l)

    biosal_shell_summarize_test_result "IntegrationTests" $passed $failed
}

main
