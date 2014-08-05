
#ifndef BSAL_COVERAGE_DISTRIBUTION_H
#define BSAL_COVERAGE_DISTRIBUTION_H

#include <engine/thorium/actor.h>

#include <core/structures/map.h>

/*
 * This is an actor for coverage distributions
 *
 * To use it:
 *
 * 1. spawn it with the script BSAL_COVERAGE_DISTRIBUTION_SCRIPT
 * 2. send BSAL_SET_EXPECTED_MESSAGE_COUNT
 * 3. Tell some other actors to send it BSAL_PUSH_DATA messages
 * 4. Enjoy
 */
struct bsal_coverage_distribution {
    struct bsal_map distribution;
    int expected;
    int actual;
    int source;
};

#define BSAL_COVERAGE_DISTRIBUTION_SCRIPT 0xfdec2b1e

#define BSAL_PUSH_DATA 0x00005c27
#define BSAL_PUSH_DATA_REPLY 0x00004874
#define BSAL_SET_EXPECTED_MESSAGE_COUNT 0x00004878
#define BSAL_SET_EXPECTED_MESSAGE_COUNT_REPLY 0x00007e2f

#define BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT "output"
#define BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE "coverage_distribution.txt"
#define BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL "coverage_distribution.txt-canonical"

extern struct bsal_script bsal_coverage_distribution_script;

void bsal_coverage_distribution_init(struct bsal_actor *actor);
void bsal_coverage_distribution_destroy(struct bsal_actor *actor);
void bsal_coverage_distribution_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_coverage_distribution_write_distribution(struct bsal_actor *self);
void bsal_coverage_distribution_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);

#endif
