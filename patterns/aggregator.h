
#ifndef BSAL_AGGREGATOR_H
#define BSAL_AGGREGATOR_H

#include <engine/actor.h>

#include <stdint.h>

#define BSAL_AGGREGATOR_SCRIPT 0x82673850

struct bsal_aggregator {
    uint64_t received;
    uint64_t last;
    int kmer_length;
};

/* message tags
 */
#define BSAL_AGGREGATE_KERNEL_OUTPUT 0x0000225f
#define BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY 0x00005cf2

extern struct bsal_script bsal_aggregator_script;

void bsal_aggregator_init(struct bsal_actor *actor);
void bsal_aggregator_destroy(struct bsal_actor *actor);
void bsal_aggregator_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
