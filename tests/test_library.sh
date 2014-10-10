#!/bin/bash

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
