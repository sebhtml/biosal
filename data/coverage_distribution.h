
#ifndef BSAL_COVERAGE_DISTRIBUTION_H
#define BSAL_COVERAGE_DISTRIBUTION_H

#include <engine/actor.h>

#include <structures/map.h>

#define BSAL_COVERAGE_DISTRIBUTION_SCRIPT 0xfdec2b1e

struct bsal_coverage_distribution {
    struct bsal_map distribution;
};

#define BSAL_PUSH_DATA 0x00005c27
#define BSAL_PUSH_DATA_REPLY 0x00004874

extern struct bsal_script bsal_coverage_distribution_script;

void bsal_coverage_distribution_init(struct bsal_actor *actor);
void bsal_coverage_distribution_destroy(struct bsal_actor *actor);
void bsal_coverage_distribution_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
