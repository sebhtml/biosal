#!/bin/bash

#maximum_test_count=1
maximum_test_count=9999

function biosal_filter_time_value()
{
    local time_value
    local seconds
    local minutes
    local hours
    local token_count
    local new_time_value

    time_value=$1

    if test $(echo $time_value | grep "s" | wc -l) -gt 0
    then
        new_time_value=$(echo $time_value|sed 's/h/ /g'|sed 's/m/ /g'|sed 's/s//g')

        hours=0
        minutes=0
        seconds=0
        token_count=$(echo "$new_time_value" | wc -w)

        #echo "token_count $token_count"

        # only seconds
        if test $token_count -eq 1
        then
            # nothing to do
            seconds=$new_time_value

        # minutes and seconds
        elif test $token_count -eq 2
        then
            seconds=$(echo "$new_time_value" | awk '{print $2}')
            minutes=$(echo "$new_time_value" | awk '{print $1}')

        # hours, minutes, seconds
        elif test $token_count -eq 3
        then
            seconds=$(echo "$new_time_value" | awk '{print $3}')
            minutes=$(echo "$new_time_value" | awk '{print $2}')
            hours=$(echo "$new_time_value" | awk '{print $1}')
        fi

        # remove decimal point
        seconds=$(echo $seconds | sed 's/\./ /g'|awk '{print $1}')

        #echo "hours $hours minutes $minutes seconds $seconds"
        time_value=$(($hours * 60 * 60 + $minutes *  60 + $seconds))
    fi

    echo $time_value
}

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

    time_value=$(biosal_filter_time_value $time_value)

    echo "        <testcase classname=\"$classname\" name=\"$name\" time=\"$time_value\">"

    if test "$error" != ""
    then
        echo "<error message=\"$error\"></error>"
    fi
    echo "        </testcase>"
}
