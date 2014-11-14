#!/bin/bash

source tests/test_library.sh

function dump_example_xml_result()
{
    local count
    local i
    local name
    local input
    local line
    local result
    local time_value_
    local error
    local total_failures

    input=$1

    count=$(cat $input | wc -l)
    total_failures=$(cat $input|grep FAILED | wc -l)

    biosal_test_junit_open_xml_stream
    biosal_test_junit_start_testsuite "application-tests" $count $total_failures

    for i in $(seq 1 $count)
    do
        # example:
        # ApplicationTest: spate-mini-1x1 Result: PASSED Time: 0m22.326s (see spate-mini-1x1.log)

        line=$(head -n $i $input | tail -n 1)

        name=$(echo $line | awk '{print $2}')
        result=$(echo $line | awk '{print $4}')
        time_value=$(echo $line | awk '{print $6}')

        error=""

        if test $result != "PASSED"
        then
            error=$line
        fi

        biosal_test_junit_emit_testcase "biosal.ApplicationTest" "$name" "$time_value" "$error"
    done

    biosal_test_junit_end_testsuite
    biosal_test_junit_close_xml_stream
}

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
    local xml_file

    xml_file="application-tests.junit.xml"

    for real_test in $(cat tests/application-tests.txt | head -n $maximum_test_count)
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

        echo "ApplicationTest: $real_test Result: $result Time: $wall_time (see $real_test.log)"

    done | tee application-tests.log

    passed=$(grep PASSED application-tests.log | wc -l)
    failed=$(grep FAILED application-tests.log | wc -l)

    biosal_shell_summarize_test_result "IntegrationTests" $passed $failed

    dump_example_xml_result application-tests.log > $xml_file

    echo "see $xml_file"

    return $failed
}

main
