#!/bin/bash

maximum_test_count=9999

function biosal_shell_summarize_test_result()
{
    local title
    local passed
    local failed
    local all

    title=$1
    passed=$2
    failed=$3
    all=$(($passed + $failed))

    echo -n $title

    echo -n " PASSED $passed / $all"

    if test $failed -gt 0
    then
        echo -n " FAILED $failed / $all"
    fi

    echo ""
}

function biosal_test_junit_open_xml_stream()
{
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
    echo "<testsuites>"
}

function get_date()
{
    # \see http://askubuntu.com/questions/355188/date-format-in-unix

    date +%Y-%m-%dT%H:%M:%S%z
}

function biosal_test_junit_start_testsuite()
{
    local timestamp
    local name
    local count
    local total_failures

    name=$1
    count=$2
    total_failures=$3

    timestamp=$(get_date)
    echo "<testsuite package=\"biosal\" name=\"$name\" tests=\"$count\" failures=\"$total_failures\" timestamp=\"$timestamp\">"
}

function biosal_test_junit_close_xml_stream()
{
    echo "</testsuites>"
}

function biosal_test_junit_end_testsuite()
{
    echo "    </testsuite>"
}

function biosal_test_junit_emit_testcase()
{
    local classname
    local name
    local time_value
    local error

    classname=$1
    name=$2
    time_value=$3
    error=$4

    echo "        <testcase classname=\"$classname\" name=\"$name\" time=\"$time_value\">"

    if test "$error" != ""
    then
        echo "<error message=\"$error\"></error>"
    fi
    echo "        </testcase>"
}
