
#ifndef BIOSAL_COVERAGE_DISTRIBUTION_H
#define BIOSAL_COVERAGE_DISTRIBUTION_H

#include <engine/thorium/actor.h>

#include <core/structures/map.h>

/*
 * This is an actor for coverage distributions
 *
 * To use it:
 *
 * 1. spawn it with the script SCRIPT_COVERAGE_DISTRIBUTION
 * 2. send ACTION_SET_EXPECTED_MESSAGE_COUNT
 * 3. Tell some other actors to send it ACTION_PUSH_DATA messages
 * 4. Enjoy
 */
struct biosal_coverage_distribution {
    struct core_map distribution;
    int expected;
    int actual;
    int source;
};

#define SCRIPT_COVERAGE_DISTRIBUTION 0xfdec2b1e

#define ACTION_PUSH_DATA 0x00005c27
#define ACTION_PUSH_DATA_REPLY 0x00004874
#define ACTION_SET_EXPECTED_MESSAGE_COUNT 0x00004878
#define ACTION_SET_EXPECTED_MESSAGE_COUNT_REPLY 0x00007e2f

#define BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE "coverage_distribution.txt"
#define BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL "coverage_distribution.txt-canonical"

extern struct thorium_script biosal_coverage_distribution_script;

#endif
