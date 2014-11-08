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
    biosal_test_junit_start_testsuite "example-tests" $count $total_failures

    for i in $(seq 1 $count)
    do
        # ExampleTest mock result: PASSED time: 0m3.765s (see mock.log)
        line=$(head -n $i $input | tail -n 1)

        name=$(echo $line | awk '{print $2}')
        result=$(echo $line | awk '{print $4}')
        time_value=$(echo $line | awk '{print $6}')

        error=""

        if test $result != "PASSED"
        then
            error=$line
        fi

        biosal_test_junit_emit_testcase "NULL" "$name" "$time_value" "$error"
    done

    biosal_test_junit_end_testsuite
    biosal_test_junit_close_xml_stream
}

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
    local xml_file

    xml_file="example-tests.junit.xml"

    for example in $(cat examples/example-tests.txt)
    do
        run_example $example
    done | tee examples.log

    passed=$(grep PASSED examples.log | wc -l)
    failed=$(grep FAILED examples.log | wc -l)

    biosal_shell_summarize_test_result "ExampleTestSuite" $passed $failed

    dump_example_xml_result examples.log > $xml_file

    echo "see $xml_file"
}

main
