#!/bin/bash

source tests/test_library.sh

# convert text version of unit test results to JUnit XML
function main()
{
    make -s test_private | tee tests.log

    cat tests.log | grep UnitTest  > tests.log.1
    count=$(cat tests.log.1 | wc -l)
    total_failures=$(cat tests.log | grep FAILED | wc -l)

    #echo "line $count"

    # \see http://help.catchsoftware.com/display/ET/JUnit+Format
    (
    biosal_test_junit_open_xml_stream

    biosal_test_junit_start_testsuite "unit-tests" $count $total_failures

    for i in $(seq 1 $count)
    do
        line=$(head -n $i tests.log.1|tail -n1)
        tests=$(echo $line | awk '{print $8}')
        failures=$(echo $line | awk '{print $6}')
        name=$(echo $line | awk '{print $2}')
        error=""

        # \see http://nelsonwells.net/2012/09/how-jenkins-ci-parses-and-displays-junit-output/

        if test $failures != "0"
        then
            error=$line
        fi

        biosal_test_junit_emit_testcase "NULL" "$name" $error

    done

    biosal_test_junit_end_testsuite
    biosal_test_junit_close_xml_stream

    ) > unit-tests.junit.xml

    tests/summarize-tests.sh tests.log

    echo "see unit-tests.junit.xml"
}

main
